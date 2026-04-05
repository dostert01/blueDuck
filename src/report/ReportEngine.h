#pragma once
#include <drogon/orm/DbClient.h>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace blueduck {

struct VersionNumber {
    std::vector<uint32_t> parts;
    std::string           pre;
    int compare(const VersionNumber& other) const;
    bool empty() const { return parts.empty(); }
};

struct MatchedCve {
    int         cve_record_id;
    std::string cve_id;
    std::string severity;
    float       cvss_score;
};

struct ReportSummary {
    int report_id;
    int total_dependencies;
    int vulnerable_count;
    int critical_count;
    int high_count;
    int medium_count;
    int low_count;
};

class ReportEngine {
public:
    explicit ReportEngine(drogon::orm::DbClientPtr db);

    int generateReport(int project_version_id);

private:
    struct DepRow {
        int         id;
        std::string ecosystem;
        std::string package_name;
        std::string package_version;
    };

    struct CpeMapping { std::string vendor; std::string product; };

    std::vector<DepRow>         loadDependencies(int project_version_id);
    std::optional<CpeMapping>   resolveCpeMapping(const std::string& ecosystem,
                                                   const std::string& package_name);
    std::vector<MatchedCve>     findMatchingCves(const DepRow& dep,
                                                  const std::optional<CpeMapping>& cpe);
    std::string   normalizePackageName(const std::string& name,
                                        const std::string& ecosystem) const;
    int  insertReport(int project_version_id);
    void insertFinding(int report_id, int dep_id,
                        int cve_record_id, const MatchedCve& cve);
    void updateReportSummary(int report_id, const ReportSummary& summary);

    drogon::orm::DbClientPtr db_;

protected:
    VersionNumber parseVersion(const std::string& version) const;
    bool versionInRange(const std::string& dep_version,
                        const std::string& exact,
                        const std::string& start_inc,
                        const std::string& start_exc,
                        const std::string& end_inc,
                        const std::string& end_exc) const;
};

} // namespace blueduck
