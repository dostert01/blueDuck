#include "CveDbWriter.h"
#include <stdexcept>
#include <cstring>
#include <format>
#include <iostream>

namespace blueduck::plugins {

CveDbWriter::CveDbWriter(const std::string& connstr) {
    conn_ = PQconnectdb(connstr.c_str());
    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "[CveDbWriter] Connection failed: " << PQerrorMessage(conn_) << "\n";
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

CveDbWriter::~CveDbWriter() {
    if (conn_) PQfinish(conn_);
}

std::string CveDbWriter::lastError() const {
    return conn_ ? PQerrorMessage(conn_) : "not connected";
}

bool CveDbWriter::upsertCve(const CveData& cve) {
    if (!conn_) return false;
    int id = upsertCveRecord(cve);
    if (id <= 0) return false;
    upsertAffectedProducts(id, cve.affected);
    return true;
}

int CveDbWriter::upsertCveRecord(const CveData& cve) {
    static const char* sql =
        "INSERT INTO cve_records"
        "(cve_id, description, severity, cvss_v3_score, cvss_v2_score, published_at, modified_at) "
        "VALUES($1,$2,$3,$4,$5,$6,$7) "
        "ON CONFLICT(cve_id) DO UPDATE SET "
        "description=EXCLUDED.description, severity=EXCLUDED.severity, "
        "cvss_v3_score=EXCLUDED.cvss_v3_score, cvss_v2_score=EXCLUDED.cvss_v2_score, "
        "modified_at=EXCLUDED.modified_at "
        "RETURNING id";

    auto v3  = cve.cvss_v3_score ? std::to_string(*cve.cvss_v3_score) : "";
    auto v2  = cve.cvss_v2_score ? std::to_string(*cve.cvss_v2_score) : "";

    const char* params[7] = {
        cve.cve_id.c_str(), cve.description.c_str(), cve.severity.c_str(),
        cve.cvss_v3_score ? v3.c_str() : nullptr,
        cve.cvss_v2_score ? v2.c_str() : nullptr,
        cve.published_at.c_str(), cve.modified_at.c_str()
    };
    int formats[7] = {0,0,0,0,0,0,0};

    PGresult* res = PQexecParams(conn_, sql, 7, nullptr, params, nullptr, formats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "[CveDbWriter] upsertCveRecord failed: " << PQerrorMessage(conn_) << "\n";
        PQclear(res);
        return -1;
    }
    int id = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return id;
}

void CveDbWriter::upsertAffectedProducts(int cve_record_id,
                                          const std::vector<AffectedRange>& affected)
{
    static const char* sql =
        "INSERT INTO cve_affected_products"
        "(cve_record_id, ecosystem, package_name, cpe_uri, "
        "version_exact, version_start_including, version_start_excluding, "
        "version_end_including, version_end_excluding) "
        "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9) "
        "ON CONFLICT DO NOTHING";

    std::string id_str = std::to_string(cve_record_id);

    for (const auto& a : affected) {
        const char* params[9] = {
            id_str.c_str(),
            a.ecosystem.c_str(),
            a.package_name.c_str(),
            a.cpe_uri.empty()           ? nullptr : a.cpe_uri.c_str(),
            a.version_exact             ? a.version_exact->c_str()     : nullptr,
            a.version_start_inc         ? a.version_start_inc->c_str() : nullptr,
            a.version_start_exc         ? a.version_start_exc->c_str() : nullptr,
            a.version_end_inc           ? a.version_end_inc->c_str()   : nullptr,
            a.version_end_exc           ? a.version_end_exc->c_str()   : nullptr
        };

        PGresult* res = PQexecParams(conn_, sql, 9, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "[CveDbWriter] upsertAffectedProducts failed: "
                      << PQerrorMessage(conn_) << "\n";
        }
        PQclear(res);
    }
}

bool CveDbWriter::exec(const std::string& sql,
                        const std::vector<std::string>& params,
                        const std::vector<bool>& nulls)
{
    std::vector<const char*> cparams;
    for (size_t i = 0; i < params.size(); ++i) {
        bool is_null = i < nulls.size() && nulls[i];
        cparams.push_back(is_null ? nullptr : params[i].c_str());
    }
    PGresult* res = PQexecParams(conn_, sql.c_str(),
                                  static_cast<int>(cparams.size()),
                                  nullptr, cparams.data(), nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK ||
               PQresultStatus(res) == PGRES_TUPLES_OK);
    if (!ok)
        std::cerr << "[CveDbWriter] exec failed: " << PQerrorMessage(conn_) << "\n";
    PQclear(res);
    return ok;
}

} // namespace blueduck::plugins
