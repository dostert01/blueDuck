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

namespace fs = std::filesystem;

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

    zip_int64_t num_entries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < num_entries; ++i) {
        const char* entry_name = zip_get_name(za, i, 0);
        if (!entry_name) continue;

        std::string name(entry_name);
        // Only process .json files under cves/ directories
        if (name.size() < 5 || name.substr(name.size() - 5) != ".json") continue;
        if (name.find("cves/") == std::string::npos) continue;

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
    }

    zip_close(za);

    std::cout << std::format("[CveOrgSource] Done: {} added, {} failed\n",
                              result.records_added, result.records_failed);
    result.success = (result.records_failed == 0);
    return result;
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
