#include "NvdSource.h"
#include "NvdParser.h"
#include <plugins/common/CveHttpClient.h>
#include <plugins/common/CveDbWriter.h>
#include <interfaces/PluginMeta.h>
#include <format>
#include <iostream>

namespace blueduck::plugins::nvd {

NvdSource::NvdSource() = default;
NvdSource::~NvdSource() = default;

void NvdSource::configure(const std::unordered_map<std::string, std::string>& cfg) {
    auto it = cfg.find("nvd_api_key");
    if (it != cfg.end()) api_key_ = it->second;

    it = cfg.find("db_connstr");
    if (it != cfg.end()) connstr_ = it->second;
}

SyncResult NvdSource::sync() {
    SyncResult result;
    if (connstr_.empty()) {
        result.error_message = "db_connstr not configured";
        return result;
    }

    CveHttpClient http;
    if (!api_key_.empty())
        http.setHeader("apiKey: " + api_key_);

    CveDbWriter writer(connstr_);
    if (!writer.isConnected()) {
        result.error_message = "database connection failed: " + writer.lastError();
        return result;
    }

    std::string first_url = std::format("{}?startIndex=0&resultsPerPage={}", BASE_URL, PAGE_SIZE);

    http.paginate(
        first_url,
        [&](const std::string& body) {
            return NvdParser::nextPageUrl(BASE_URL, PAGE_SIZE, body);
        },
        [&](const std::string& body) -> bool {
            auto cves = NvdParser::parse(body);
            for (const auto& cve : cves) {
                if (writer.upsertCve(cve))
                    ++result.records_added;
                else
                    ++result.records_failed;
            }
            std::cout << std::format("[NvdSource] processed {} CVEs so far\n",
                                      result.records_added + result.records_failed);
            return true;
        });

    result.success = (result.records_failed == 0);
    return result;
}

} // namespace blueduck::plugins::nvd

// ---------- Plugin entry points ----------

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "cve_source", "nvd", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::nvd::NvdSource();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::nvd::NvdSource*>(p);
}

} // extern "C"
