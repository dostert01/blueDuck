#include "ReportController.h"
#include <api/ApiHelpers.h>
#include <db/Database.h>
#include <db/FieldHelpers.h>
#include <report/ReportEngine.h>
#include <drogon/drogon.h>

namespace blueduck {

namespace {

Json::Value reportRowToJson(const drogon::orm::Row& row) {
    Json::Value o;
    o["id"]                 = row["id"].as<int>();
    o["project_version_id"] = row["project_version_id"].as<int>();
    o["created_at"]         = row["created_at"].as<std::string>();
    o["total_dependencies"] = fieldOr(row["total_dependencies"], 0);
    o["vulnerable_count"]   = fieldOr(row["vulnerable_count"], 0);
    o["critical_count"]     = fieldOr(row["critical_count"], 0);
    o["high_count"]         = fieldOr(row["high_count"], 0);
    o["medium_count"]       = fieldOr(row["medium_count"], 0);
    o["low_count"]          = fieldOr(row["low_count"], 0);
    return o;
}

Json::Value findingRowToJson(const drogon::orm::Row& row) {
    Json::Value o;
    o["id"]            = row["id"].as<int>();
    o["dependency_id"] = row["dependency_id"].as<int>();
    o["ecosystem"]     = row["ecosystem"].as<std::string>();
    o["package_name"]  = row["package_name"].as<std::string>();
    o["package_version"] = fieldOrEmpty(row["package_version"]);
    o["cve_id"]        = row["cve_id"].as<std::string>();
    o["severity"]      = fieldOrEmpty(row["severity"]);
    o["cvss_score"]    = fieldOr(row["cvss_score"], 0.0f);
    return o;
}

} // anonymous namespace

void ReportController::listForVersion(const drogon::HttpRequestPtr&,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                       int project_id, int version_id)
{
    auto db = Database::client();
    *db << "SELECT r.id, r.project_version_id, r.created_at, "
           "r.total_dependencies, r.vulnerable_count, "
           "r.critical_count, r.high_count, r.medium_count, r.low_count "
           "FROM reports r "
           "JOIN project_versions pv ON pv.id = r.project_version_id "
           "WHERE pv.project_id=$1 AND pv.id=$2 "
           "ORDER BY r.created_at DESC"
        << project_id << version_id
        >> [cb](const drogon::orm::Result& r) {
               Json::Value arr(Json::arrayValue);
               for (const auto& row : r)
                   arr.append(reportRowToJson(row));
               cb(ApiHelpers::ok(arr));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ReportController::get(const drogon::HttpRequestPtr&,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                            int id)
{
    auto db = Database::client();
    *db << "SELECT id, project_version_id, created_at, "
           "total_dependencies, vulnerable_count, "
           "critical_count, high_count, medium_count, low_count "
           "FROM reports WHERE id=$1"
        << id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("report"));
               cb(ApiHelpers::ok(reportRowToJson(r[0])));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ReportController::findings(const drogon::HttpRequestPtr&,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 int id)
{
    auto db = Database::client();
    *db << "SELECT rf.id, rf.dependency_id, "
           "d.ecosystem, d.package_name, COALESCE(d.package_version,'') AS package_version, "
           "cr.cve_id, rf.severity, rf.cvss_score "
           "FROM report_findings rf "
           "JOIN dependencies d ON d.id = rf.dependency_id "
           "JOIN cve_records cr ON cr.id = rf.cve_record_id "
           "WHERE rf.report_id=$1 "
           "ORDER BY rf.cvss_score DESC, cr.cve_id"
        << id
        >> [cb](const drogon::orm::Result& r) {
               Json::Value arr(Json::arrayValue);
               for (const auto& row : r)
                   arr.append(findingRowToJson(row));
               cb(ApiHelpers::ok(arr));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ReportController::dependencies(const drogon::HttpRequestPtr&,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                     int id)
{
    auto db = Database::client();
    *db << "SELECT d.id, d.ecosystem, d.package_name, "
           "COALESCE(d.package_version,'') AS package_version "
           "FROM dependencies d "
           "JOIN reports r ON r.project_version_id = d.project_version_id "
           "WHERE r.id=$1 "
           "ORDER BY d.ecosystem, d.package_name"
        << id
        >> [cb](const drogon::orm::Result& r) {
               Json::Value arr(Json::arrayValue);
               for (const auto& row : r) {
                   Json::Value o;
                   o["id"]              = row["id"].as<int>();
                   o["ecosystem"]       = row["ecosystem"].as<std::string>();
                   o["package_name"]    = row["package_name"].as<std::string>();
                   o["package_version"] = row["package_version"].as<std::string>();
                   arr.append(o);
               }
               cb(ApiHelpers::ok(arr));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ReportController::generate(const drogon::HttpRequestPtr&,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  int project_id, int version_id)
{
    auto db = Database::client();
    *db << "SELECT pv.id FROM project_versions pv "
           "WHERE pv.project_id=$1 AND pv.id=$2"
        << project_id << version_id
        >> [cb, version_id](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("version"));

               drogon::async_run([version_id]() -> drogon::Task<> {
                   ReportEngine engine(Database::blockingClient());
                   engine.generateReport(version_id);
                   co_return;
               });

               Json::Value resp; resp["status"] = "report generation started";
               cb(ApiHelpers::accepted(resp));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

} // namespace blueduck
