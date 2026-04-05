#pragma once
#include <string>
#include <vector>
#include <optional>
#include <libpq-fe.h>

namespace blueduck::plugins {

/// Writes CVE data into the blueDuck PostgreSQL schema using libpq directly.
/// Plugins use this instead of Drogon ORM since they don't link Drogon.
class CveDbWriter {
public:
    explicit CveDbWriter(const std::string& connstr);
    ~CveDbWriter();

    struct AffectedRange {
        std::string ecosystem;
        std::string package_name;
        std::string cpe_uri;
        std::optional<std::string> version_exact;
        std::optional<std::string> version_start_inc;
        std::optional<std::string> version_start_exc;
        std::optional<std::string> version_end_inc;
        std::optional<std::string> version_end_exc;
    };

    struct CveData {
        std::string cve_id;          ///< e.g. "CVE-2023-12345"
        std::string description;
        std::string severity;        ///< CRITICAL / HIGH / MEDIUM / LOW / NONE
        std::optional<float> cvss_v3_score;
        std::optional<float> cvss_v2_score;
        std::string published_at;    ///< ISO 8601
        std::string modified_at;
        std::vector<AffectedRange> affected;
    };

    /// Upsert a single CVE record and all its affected-product rows.
    /// Returns true on success.
    bool upsertCve(const CveData& cve);

    bool isConnected() const { return conn_ != nullptr; }
    std::string lastError() const;

private:
    PGconn* conn_{nullptr};

    int upsertCveRecord(const CveData& cve);
    void upsertAffectedProducts(int cve_record_id, const std::vector<AffectedRange>& affected);

    /// Execute a parameterised query; returns true on success.
    bool exec(const std::string& sql,
              const std::vector<std::string>& params,
              const std::vector<bool>& nulls = {});
};

} // namespace blueduck::plugins
