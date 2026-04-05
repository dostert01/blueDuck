#include "NvdParser.h"
#include <nlohmann/json.hpp>
#include <format>
#include <iostream>

namespace blueduck::plugins::nvd {

using json = nlohmann::json;

std::vector<CveDbWriter::CveData> NvdParser::parse(const std::string& json_body) {
    std::vector<CveDbWriter::CveData> results;

    json root;
    try { root = json::parse(json_body); }
    catch (const std::exception& e) {
        std::cerr << "[NvdParser] JSON parse error: " << e.what() << "\n";
        return results;
    }

    if (!root.contains("vulnerabilities")) return results;

    for (const auto& vuln : root["vulnerabilities"]) {
        const auto& cve_node = vuln["cve"];

        CveDbWriter::CveData cve;
        cve.cve_id = cve_node.value("id", "");
        if (cve.cve_id.empty()) continue;

        // Description (English preferred)
        if (cve_node.contains("descriptions")) {
            for (const auto& d : cve_node["descriptions"]) {
                if (d.value("lang", "") == "en") {
                    cve.description = d.value("value", "");
                    break;
                }
            }
        }

        cve.published_at = cve_node.value("published", "");
        cve.modified_at  = cve_node.value("lastModified", "");

        // CVSS scores
        if (cve_node.contains("metrics")) {
            const auto& m = cve_node["metrics"];

            if (m.contains("cvssMetricV31") && !m["cvssMetricV31"].empty()) {
                const auto& cv31 = m["cvssMetricV31"][0];
                if (cv31.contains("cvssData")) {
                    float score = cv31["cvssData"].value("baseScore", 0.0f);
                    cve.cvss_v3_score = score;
                    cve.severity = severityFromCvssV3(score);
                }
            } else if (m.contains("cvssMetricV30") && !m["cvssMetricV30"].empty()) {
                const auto& cv30 = m["cvssMetricV30"][0];
                if (cv30.contains("cvssData")) {
                    float score = cv30["cvssData"].value("baseScore", 0.0f);
                    cve.cvss_v3_score = score;
                    cve.severity = severityFromCvssV3(score);
                }
            } else if (m.contains("cvssMetricV2") && !m["cvssMetricV2"].empty()) {
                const auto& cv2 = m["cvssMetricV2"][0];
                if (cv2.contains("cvssData")) {
                    float score = cv2["cvssData"].value("baseScore", 0.0f);
                    cve.cvss_v2_score = score;
                    cve.severity = severityFromCvssV2(score);
                }
            }
        }

        // Affected products / CPE configurations
        if (cve_node.contains("configurations")) {
            for (const auto& config : cve_node["configurations"]) {
                if (!config.contains("nodes")) continue;
                for (const auto& node : config["nodes"]) {
                    if (!node.contains("cpeMatch")) continue;
                    for (const auto& match : node["cpeMatch"]) {
                        if (!match.value("vulnerable", false)) continue;

                        CveDbWriter::AffectedRange r;
                        r.cpe_uri      = match.value("criteria", "");
                        r.ecosystem    = "";
                        r.package_name = "";

                        if (match.contains("versionExact"))
                            r.version_exact = match["versionExact"].get<std::string>();
                        if (match.contains("versionStartIncluding"))
                            r.version_start_inc = match["versionStartIncluding"].get<std::string>();
                        if (match.contains("versionStartExcluding"))
                            r.version_start_exc = match["versionStartExcluding"].get<std::string>();
                        if (match.contains("versionEndIncluding"))
                            r.version_end_inc = match["versionEndIncluding"].get<std::string>();
                        if (match.contains("versionEndExcluding"))
                            r.version_end_exc = match["versionEndExcluding"].get<std::string>();

                        cve.affected.push_back(std::move(r));
                    }
                }
            }
        }

        results.push_back(std::move(cve));
    }

    return results;
}

std::string NvdParser::nextPageUrl(const std::string& base_url,
                                    int page_size,
                                    const std::string& json_body)
{
    json root;
    try { root = json::parse(json_body); }
    catch (...) { return ""; }

    int total     = root.value("totalResults", 0);
    int start     = root.value("startIndex",   0);
    int returned  = root.value("resultsPerPage", 0);
    int next      = start + returned;

    if (next >= total) return "";
    return std::format("{}?startIndex={}&resultsPerPage={}", base_url, next, page_size);
}

std::string NvdParser::severityFromCvssV3(float score) {
    if (score >= 9.0f) return "CRITICAL";
    if (score >= 7.0f) return "HIGH";
    if (score >= 4.0f) return "MEDIUM";
    if (score > 0.0f)  return "LOW";
    return "NONE";
}

std::string NvdParser::severityFromCvssV2(float score) {
    if (score >= 7.0f) return "HIGH";
    if (score >= 4.0f) return "MEDIUM";
    if (score > 0.0f)  return "LOW";
    return "NONE";
}

} // namespace blueduck::plugins::nvd
