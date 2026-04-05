#include "ReportEngine.h"
#include <db/FieldHelpers.h>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <format>
#include <iostream>
#include <cctype>

namespace blueduck {

int VersionNumber::compare(const VersionNumber& other) const {
    size_t len = std::max(parts.size(), other.parts.size());
    for (size_t i = 0; i < len; ++i) {
        uint32_t a = i < parts.size()       ? parts[i]       : 0;
        uint32_t b = i < other.parts.size() ? other.parts[i] : 0;
        if (a < b) return -1;
        if (a > b) return  1;
    }
    if (!pre.empty() &&  other.pre.empty()) return -1;
    if ( pre.empty() && !other.pre.empty()) return  1;
    return pre.compare(other.pre);
}

ReportEngine::ReportEngine(drogon::orm::DbClientPtr db)
    : db_(std::move(db)) {}

int ReportEngine::generateReport(int project_version_id) {
    std::cout << std::format("[ReportEngine] Generating report for version {}\n",
                              project_version_id);

    int report_id = insertReport(project_version_id);
    auto deps     = loadDependencies(project_version_id);

    ReportSummary summary{};
    summary.report_id          = report_id;
    summary.total_dependencies = static_cast<int>(deps.size());

    for (const auto& dep : deps) {
        auto cpe  = resolveCpeMapping(dep.ecosystem, dep.package_name);
        auto cves = findMatchingCves(dep, cpe);

        for (const auto& cve : cves) {
            insertFinding(report_id, dep.id, cve.cve_record_id, cve);
            ++summary.vulnerable_count;
            if      (cve.severity == "CRITICAL") ++summary.critical_count;
            else if (cve.severity == "HIGH")     ++summary.high_count;
            else if (cve.severity == "MEDIUM")   ++summary.medium_count;
            else if (cve.severity == "LOW")      ++summary.low_count;
        }
    }

    updateReportSummary(report_id, summary);
    std::cout << std::format("[ReportEngine] Report {} complete: {} deps, {} vulnerable\n",
                              report_id, summary.total_dependencies, summary.vulnerable_count);
    return report_id;
}

std::vector<ReportEngine::DepRow>
ReportEngine::loadDependencies(int project_version_id) {
    std::vector<DepRow> deps;
    auto r = db_->execSqlSync(
        "SELECT id, ecosystem, package_name, COALESCE(package_version,'') "
        "FROM dependencies WHERE project_version_id = $1",
        project_version_id);
    for (const auto& row : r)
        deps.push_back({ row[0].as<int>(), row[1].as<std::string>(),
                         row[2].as<std::string>(), row[3].as<std::string>() });
    return deps;
}

std::optional<ReportEngine::CpeMapping>
ReportEngine::resolveCpeMapping(const std::string& ecosystem,
                                 const std::string& package_name)
{
    auto r = db_->execSqlSync(
        "SELECT cpe_vendor, cpe_product FROM ecosystem_cpe_mapping "
        "WHERE ecosystem = $1 AND package_name = $2 LIMIT 1",
        ecosystem, package_name);
    if (!r.empty())
        return CpeMapping{ r[0][0].as<std::string>(), r[0][1].as<std::string>() };

    r = db_->execSqlSync(
        "SELECT cpe_vendor, cpe_product FROM ecosystem_cpe_mapping "
        "WHERE ecosystem = $1 AND git_url_pattern IS NOT NULL "
        "AND $2 ILIKE '%' || git_url_pattern || '%' LIMIT 1",
        ecosystem, package_name);
    if (!r.empty())
        return CpeMapping{ r[0][0].as<std::string>(), r[0][1].as<std::string>() };

    return std::nullopt;
}

std::vector<MatchedCve>
ReportEngine::findMatchingCves(const DepRow& dep,
                                const std::optional<CpeMapping>& cpe)
{
    std::vector<MatchedCve> matches;

    static const std::string query = R"(
        SELECT cap.cve_record_id,
               cap.version_exact,
               cap.version_start_including,
               cap.version_start_excluding,
               cap.version_end_including,
               cap.version_end_excluding,
               cr.cve_id,
               cr.severity,
               COALESCE(cr.cvss_v3_score, cr.cvss_v2_score, 0.0)
        FROM cve_affected_products cap
        JOIN cve_records cr ON cr.id = cap.cve_record_id
        WHERE (
            (cap.ecosystem = $1 AND LOWER(cap.package_name) = LOWER($2))
            OR
            ($3 != '' AND LOWER(cap.cpe_uri) LIKE '%:' || LOWER($3) || ':' || LOWER($4) || ':%')
        )
    )";

    std::string cpe_vendor  = cpe ? cpe->vendor  : "";
    std::string cpe_product = cpe ? cpe->product : "";

    auto r = db_->execSqlSync(query,
        dep.ecosystem,
        normalizePackageName(dep.package_name, dep.ecosystem),
        cpe_vendor, cpe_product);

    for (const auto& row : r) {
        bool any_version =
            fieldOrEmpty(row[1]).empty() &&
            fieldOrEmpty(row[2]).empty() &&
            fieldOrEmpty(row[3]).empty() &&
            fieldOrEmpty(row[4]).empty() &&
            fieldOrEmpty(row[5]).empty();

        if (!any_version && dep.package_version.empty()) continue;

        if (any_version || versionInRange(dep.package_version,
                fieldOrEmpty(row[1]),
                fieldOrEmpty(row[2]),
                fieldOrEmpty(row[3]),
                fieldOrEmpty(row[4]),
                fieldOrEmpty(row[5])))
        {
            matches.push_back({ row[0].as<int>(),
                                row[6].as<std::string>(),
                                fieldOrEmpty(row[7]),
                                fieldOr(row[8], 0.0f) });
        }
    }

    std::sort(matches.begin(), matches.end(),
        [](const MatchedCve& a, const MatchedCve& b) {
            return a.cve_record_id < b.cve_record_id;
        });
    matches.erase(std::unique(matches.begin(), matches.end(),
        [](const MatchedCve& a, const MatchedCve& b) {
            return a.cve_record_id == b.cve_record_id;
        }), matches.end());

    return matches;
}

int ReportEngine::insertReport(int project_version_id) {
    auto r = db_->execSqlSync(
        "INSERT INTO reports(project_version_id) VALUES($1) RETURNING id",
        project_version_id);
    return r[0][0].as<int>();
}

void ReportEngine::insertFinding(int report_id, int dep_id,
                                   int cve_record_id, const MatchedCve& cve)
{
    db_->execSqlSync(
        "INSERT INTO report_findings"
        "(report_id, dependency_id, cve_record_id, cvss_score, severity)"
        "VALUES($1,$2,$3,$4,$5) ON CONFLICT DO NOTHING",
        report_id, dep_id, cve_record_id, cve.cvss_score, cve.severity);
}

void ReportEngine::updateReportSummary(int report_id, const ReportSummary& s) {
    db_->execSqlSync(
        "UPDATE reports SET total_dependencies=$2, vulnerable_count=$3,"
        " critical_count=$4, high_count=$5, medium_count=$6, low_count=$7"
        " WHERE id=$1",
        report_id,
        s.total_dependencies, s.vulnerable_count,
        s.critical_count, s.high_count,
        s.medium_count, s.low_count);
}

VersionNumber ReportEngine::parseVersion(const std::string& version) const {
    VersionNumber v;
    if (version.empty()) return v;
    std::string s = (version[0] == 'v' || version[0] == 'V') ? version.substr(1) : version;
    auto sep = s.find_first_of("-+");
    if (sep != std::string::npos) { v.pre = s.substr(sep + 1); s = s.substr(0, sep); }
    std::istringstream ss(s);
    std::string part;
    while (std::getline(ss, part, '.')) {
        try { v.parts.push_back(static_cast<uint32_t>(std::stoul(part))); }
        catch (...) { break; }
    }
    return v;
}

bool ReportEngine::versionInRange(const std::string& dep_version,
                                   const std::string& exact,
                                   const std::string& start_inc,
                                   const std::string& start_exc,
                                   const std::string& end_inc,
                                   const std::string& end_exc) const
{
    if (dep_version.empty()) return false;
    VersionNumber v = parseVersion(dep_version);
    if (v.empty()) return false;
    if (!exact.empty())     return v.compare(parseVersion(exact)) == 0;
    if (!start_inc.empty() && v.compare(parseVersion(start_inc)) < 0)  return false;
    if (!start_exc.empty() && v.compare(parseVersion(start_exc)) <= 0) return false;
    if (!end_inc.empty()   && v.compare(parseVersion(end_inc))   > 0)  return false;
    if (!end_exc.empty()   && v.compare(parseVersion(end_exc))   >= 0) return false;
    return true;
}

std::string ReportEngine::normalizePackageName(const std::string& name,
                                                const std::string& ecosystem) const
{
    if (ecosystem == "pypi") {
        std::string n = name;
        std::transform(n.begin(), n.end(), n.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        std::replace(n.begin(), n.end(), '-', '_');
        return n;
    }
    if (ecosystem == "nuget") {
        std::string n = name;
        std::transform(n.begin(), n.end(), n.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        return n;
    }
    return name;
}

} // namespace blueduck
