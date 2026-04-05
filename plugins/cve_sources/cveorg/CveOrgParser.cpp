#include "CveOrgParser.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace blueduck::plugins::cveorg {

using json = nlohmann::json;

std::optional<CveDbWriter::CveData> CveOrgParser::parse(const std::string& json_body) {
    json root;
    try { root = json::parse(json_body); }
    catch (const std::exception& e) {
        std::cerr << "[CveOrgParser] JSON parse error: " << e.what() << "\n";
        return std::nullopt;
    }

    // CVE 5.0 format: root["cveMetadata"]["cveId"]
    if (!root.contains("cveMetadata")) return std::nullopt;
    const auto& meta = root["cveMetadata"];

    CveDbWriter::CveData cve;
    cve.cve_id       = meta.value("cveId", "");
    cve.published_at = meta.value("datePublished", "");
    cve.modified_at  = meta.value("dateUpdated", "");

    if (cve.cve_id.empty()) return std::nullopt;

    // Description from containers.cna.descriptions
    if (root.contains("containers") && root["containers"].contains("cna")) {
        const auto& cna = root["containers"]["cna"];

        if (cna.contains("descriptions")) {
            for (const auto& d : cna["descriptions"]) {
                if (d.value("lang", "") == "en") {
                    cve.description = d.value("value", "");
                    break;
                }
            }
            if (cve.description.empty() && !cna["descriptions"].empty())
                cve.description = cna["descriptions"][0].value("value", "");
        }

        // CVSS scores from metrics
        if (cna.contains("metrics")) {
            for (const auto& metric : cna["metrics"]) {
                if (metric.contains("cvssV3_1")) {
                    float score = metric["cvssV3_1"].value("baseScore", 0.0f);
                    cve.cvss_v3_score = score;
                    cve.severity      = metric["cvssV3_1"].value("baseSeverity", "");
                    break;
                }
                if (metric.contains("cvssV3_0")) {
                    float score = metric["cvssV3_0"].value("baseScore", 0.0f);
                    cve.cvss_v3_score = score;
                    cve.severity      = metric["cvssV3_0"].value("baseSeverity", "");
                    break;
                }
                if (metric.contains("cvssV2_0")) {
                    float score = metric["cvssV2_0"].value("baseScore", 0.0f);
                    cve.cvss_v2_score = score;
                    // CVSSv2 has no baseSeverity field in some records
                }
            }
        }

        // Affected packages
        if (cna.contains("affected")) {
            for (const auto& affected_entry : cna["affected"]) {
                std::string ecosystem = affected_entry.value("packageName", "");
                std::string pkg       = affected_entry.value("product", "");
                std::string cpe       = affected_entry.value("cpes", json::array()).empty()
                                        ? ""
                                        : affected_entry["cpes"][0].get<std::string>();

                if (!affected_entry.contains("versions")) {
                    CveDbWriter::AffectedRange r;
                    r.ecosystem    = ecosystem;
                    r.package_name = pkg;
                    r.cpe_uri      = cpe;
                    cve.affected.push_back(std::move(r));
                    continue;
                }

                for (const auto& ver : affected_entry["versions"]) {
                    CveDbWriter::AffectedRange r;
                    r.ecosystem    = ecosystem;
                    r.package_name = pkg;
                    r.cpe_uri      = cpe;

                    std::string status = ver.value("status", "");
                    if (status != "affected") continue;

                    std::string ver_str = ver.value("version", "");
                    std::string vtype   = ver.value("versionType", "semver");
                    std::string lt      = ver.value("lessThan", "");
                    std::string lte     = ver.value("lessThanOrEqual", "");

                    if (!lt.empty() || !lte.empty()) {
                        r.version_start_inc = ver_str.empty() ? std::nullopt
                                                               : std::make_optional(ver_str);
                        if (!lt.empty())  r.version_end_exc = lt;
                        if (!lte.empty()) r.version_end_inc = lte;
                    } else if (!ver_str.empty() && ver_str != "0") {
                        r.version_exact = ver_str;
                    }

                    cve.affected.push_back(std::move(r));
                }
            }
        }
    }

    return cve;
}

} // namespace blueduck::plugins::cveorg
