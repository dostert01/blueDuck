#include "VersionController.h"
#include <api/ApiHelpers.h>
#include <db/Database.h>
#include <db/FieldHelpers.h>
#include <core/Application.h>
#include <drogon/drogon.h>

namespace blueduck {

namespace {

Json::Value versionRowToJson(const drogon::orm::Row& row) {
    Json::Value o;
    o["id"]           = row["id"].as<int>();
    o["project_id"]   = row["project_id"].as<int>();
    o["ref_name"]     = row["ref_name"].as<std::string>();
    o["ref_type"]     = row["ref_type"].as<std::string>();
    o["commit_hash"]  = fieldOrEmpty(row["commit_hash"]);
    o["analyzed_at"]  = row["analyzed_at"].isNull()
                        ? Json::Value()
                        : Json::Value(row["analyzed_at"].as<std::string>());
    o["dep_count"]    = fieldOr(row["dep_count"], 0);
    return o;
}

} // anonymous namespace

void VersionController::list(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              int project_id)
{
    auto db = Database::client();
    *db << "SELECT pv.id, pv.project_id, pv.ref_name, pv.ref_type, pv.commit_hash, "
           "pv.analyzed_at, COUNT(d.id)::int AS dep_count "
           "FROM project_versions pv "
           "LEFT JOIN dependencies d ON d.project_version_id = pv.id "
           "WHERE pv.project_id = $1 "
           "GROUP BY pv.id ORDER BY pv.ref_name"
        << project_id
        >> [cb](const drogon::orm::Result& r) {
               Json::Value arr(Json::arrayValue);
               for (const auto& row : r)
                   arr.append(versionRowToJson(row));
               cb(ApiHelpers::ok(arr));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void VersionController::get(const drogon::HttpRequestPtr&,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             int project_id, int id)
{
    auto db = Database::client();
    *db << "SELECT pv.id, pv.project_id, pv.ref_name, pv.ref_type, pv.commit_hash, "
           "pv.analyzed_at, COUNT(d.id)::int AS dep_count "
           "FROM project_versions pv "
           "LEFT JOIN dependencies d ON d.project_version_id = pv.id "
           "WHERE pv.project_id = $1 AND pv.id = $2 "
           "GROUP BY pv.id"
        << project_id << id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("version"));
               cb(ApiHelpers::ok(versionRowToJson(r[0])));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void VersionController::remove(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                int project_id, int id)
{
    auto db = Database::client();
    *db << "DELETE FROM project_versions WHERE project_id=$1 AND id=$2 RETURNING id"
        << project_id << id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("version"));
               cb(ApiHelpers::noContent());
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void VersionController::analyze(const drogon::HttpRequestPtr&,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 int project_id, int id)
{
    auto db = Database::client();
    *db << "SELECT pv.id FROM project_versions pv WHERE pv.project_id=$1 AND pv.id=$2"
        << project_id << id
        >> [cb, id](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("version"));

               drogon::async_run([id]() -> drogon::Task<> {
                   Application::instance().analyzeVersion(id);
                   co_return;
               });

               Json::Value resp; resp["status"] = "analysis started";
               cb(ApiHelpers::accepted(resp));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

} // namespace blueduck
