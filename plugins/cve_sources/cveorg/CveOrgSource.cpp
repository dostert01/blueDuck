#include "CveOrgSource.h"
#include "CveOrgParser.h"
#include <plugins/common/CveHttpClient.h>
#include <plugins/common/CveDbWriter.h>
#include <interfaces/PluginMeta.h>
#include <zip.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <format>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool endsWithCi(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return toLower(s.substr(s.size() - suffix.size())) == toLower(suffix);
}

// Maximum nesting depth for zip-within-zip archives (CVEProject's "all
// CVEs" release wraps its data one level deep as of this writing).
// Bounded defensively in case of malformed/malicious archives.
constexpr int kMaxZipNestingDepth = 4;

} // namespace

namespace blueduck::plugins::cveorg {

CveOrgSource::CveOrgSource() = default;
CveOrgSource::~CveOrgSource() = default;

void CveOrgSource::configure(const std::unordered_map<std::string, std::string>& cfg) {
    auto it = cfg.find("db_connstr");
    if (it != cfg.end()) connstr_ = it->second;

    it = cfg.find("cache_dir");
    if (it != cfg.end()) cache_dir_ = it->second;
    else cache_dir_ = "/tmp/blueduck_cveorg";

    fs::create_directories(cache_dir_);
}

bool CveOrgSource::downloadFile(const std::string& url, const std::string& dest_path) {
    CveHttpClient http;
    auto resp = http.get(url);
    if (!resp.ok()) {
        std::cerr << "[CveOrgSource] download failed " << resp.status_code
                  << " " << resp.error << "\n";
        return false;
    }
    std::ofstream out(dest_path, std::ios::binary);
    out.write(resp.body.data(), static_cast<std::streamsize>(resp.body.size()));
    return out.good();
}

CveOrgSource::ReleaseAssets CveOrgSource::discoverAssetUrls() {
    ReleaseAssets assets;
    CveHttpClient http;
    http.setHeader("Accept: application/vnd.github+json");
    http.setHeader("User-Agent: blueDuck-CVE-Scanner");

    auto resp = http.get(RELEASES_API_URL);
    if (!resp.ok()) {
        std::cerr << "[CveOrgSource] Failed to query GitHub Releases API: "
                  << resp.status_code << "\n";
        return assets;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        for (const auto& asset : j["assets"]) {
            std::string name = asset["name"].get<std::string>();
            std::string url  = asset["browser_download_url"].get<std::string>();
            if (name.find("delta") != std::string::npos && name.ends_with(".zip"))
                assets.delta_url = url;
            else if (name.find("all_CVEs") != std::string::npos && name.ends_with(".zip"))
                assets.full_url = url;
        }
    } catch (const std::exception& e) {
        std::cerr << "[CveOrgSource] Failed to parse release JSON: " << e.what() << "\n";
    }

    return assets;
}

SyncResult CveOrgSource::sync() {
    SyncResult result;
    if (connstr_.empty()) {
        result.error_message = "db_connstr not configured";
        return result;
    }

    auto assets = discoverAssetUrls();
    if (assets.delta_url.empty() && assets.full_url.empty()) {
        result.error_message = "could not discover download URLs from GitHub Releases API";
        return result;
    }

    std::string zip_path;
    bool downloaded = false;

    if (!assets.delta_url.empty()) {
        zip_path = cache_dir_ + "/delta.zip";
        std::cout << "[CveOrgSource] Downloading delta feed...\n";
        downloaded = downloadFile(assets.delta_url, zip_path);
    }

    if (!downloaded) {
        if (assets.full_url.empty()) {
            result.error_message = "failed to download CVE data (no full download URL available)";
            return result;
        }
        std::cout << "[CveOrgSource] Delta failed or unavailable, trying full download...\n";
        zip_path = cache_dir_ + "/allCVEs.zip";
        if (!downloadFile(assets.full_url, zip_path)) {
            result.error_message = "failed to download CVE data";
            return result;
        }
    }

    return processBulkZip(zip_path);
}

SyncResult CveOrgSource::processBulkZip(const std::string& zip_path) {
    SyncResult result;

    int err = 0;
    zip_t* za = zip_open(zip_path.c_str(), ZIP_RDONLY, &err);
    if (!za) {
        result.error_message = std::format("zip_open failed: error {}", err);
        return result;
    }

    CveDbWriter writer(connstr_);
    if (!writer.isConnected()) {
        result.error_message = "database connection failed: " + writer.lastError();
        zip_close(za);
        return result;
    }

    processZipEntries(za, writer, result);

    zip_close(za);

    std::cout << std::format("[CveOrgSource] Done: {} added, {} failed\n",
                              result.records_added, result.records_failed);
    // A handful of bad individual records shouldn't sink an otherwise-productive
    // sync; only report failure when nothing was imported at all.
    result.success = result.records_added > 0 || result.records_failed == 0;
    if (result.records_failed > 0)
        result.error_message = std::format("{} record(s) failed to upsert (see logs)",
                                            result.records_failed);
    return result;
}

void CveOrgSource::processZipEntries(zip_t* za, CveDbWriter& writer,
                                      SyncResult& result, int depth) {
    if (depth > kMaxZipNestingDepth) {
        std::cerr << "[CveOrgSource] zip nesting depth limit exceeded, aborting descent\n";
        return;
    }

    zip_int64_t num_entries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < num_entries; ++i) {
        const char* entry_name = zip_get_name(za, i, 0);
        if (!entry_name) continue;

        std::string name(entry_name);

        if (endsWithCi(name, ".json")) {
            // CVEProject releases nest CVE JSON files under directories whose
            // casing varies ("cves/", "deltaCves/", ...); match case-insensitively
            // rather than relying on one exact spelling.
            if (toLower(name).find("cves/") == std::string::npos) continue;

            zip_file_t* zf = zip_fopen_index(za, i, 0);
            if (!zf) continue;

            std::string content;
            char buf[8192];
            zip_int64_t n;
            while ((n = zip_fread(zf, buf, sizeof(buf))) > 0)
                content.append(buf, static_cast<size_t>(n));
            zip_fclose(zf);

            auto cve = CveOrgParser::parse(content);
            if (cve) {
                if (writer.upsertCve(*cve))
                    ++result.records_added;
                else
                    ++result.records_failed;
            }
        } else if (endsWithCi(name, ".zip")) {
            // CVEProject's "all CVEs" release asset is itself a zip whose
            // single entry is another zip (e.g. "cves.zip") containing the
            // actual per-CVE JSON files. Read it into memory and recurse.
            zip_file_t* zf = zip_fopen_index(za, i, 0);
            if (!zf) continue;

            std::string nested;
            char buf[1 << 16];
            zip_int64_t n;
            while ((n = zip_fread(zf, buf, sizeof(buf))) > 0)
                nested.append(buf, static_cast<size_t>(n));
            zip_fclose(zf);

            zip_error_t zerr;
            zip_error_init(&zerr);
            zip_source_t* src = zip_source_buffer_create(nested.data(), nested.size(), 0, &zerr);
            if (!src) {
                std::cerr << "[CveOrgSource] zip_source_buffer_create failed for "
                          << name << ": " << zip_error_strerror(&zerr) << "\n";
                zip_error_fini(&zerr);
                continue;
            }

            zip_t* nested_za = zip_open_from_source(src, ZIP_RDONLY, &zerr);
            if (!nested_za) {
                std::cerr << "[CveOrgSource] zip_open_from_source failed for "
                          << name << ": " << zip_error_strerror(&zerr) << "\n";
                zip_source_free(src);
                zip_error_fini(&zerr);
                continue;
            }
            zip_error_fini(&zerr);

            processZipEntries(nested_za, writer, result, depth + 1);
            zip_close(nested_za); // also releases src
        }
    }
}

} // namespace blueduck::plugins::cveorg

// ---------- Plugin entry points ----------

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "cve_source", "cveorg", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::cveorg::CveOrgSource();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::cveorg::CveOrgSource*>(p);
}

} // extern "C"
