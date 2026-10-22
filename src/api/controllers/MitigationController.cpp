#include "MitigationController.h"
#include <api/ApiHelpers.h>
#include <db/Database.h>
#include <db/FieldHelpers.h>

namespace blueduck {

namespace {

Json::Value mitigationToJson(const drogon::orm::Row& row) {
    Json::Value o;
    o["id"]           = row["id"].as<int>();
    o["finding_id"]   = row["finding_id"].as<int>();
    o["type"]         = row["type"].as<std::string>();
    o["description"]  = fieldOrEmpty(row["description"]);
    o["mitigated_at"] = row["mitigated_at"].as<std::string>();
    return o;
}

} // anonymous namespace

void MitigationController::create(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                    int finding_id)
{
    auto body = req->getJsonObject();
    if (!body || !(*body).isMember("type"))
        return cb(ApiHelpers::badRequest("type required"));

    std::string type = (*body)["type"].asString();
    if (type != "NOT_APPLICABLE" && type != "FALSE_FINDING" && type != "MITIGATED_BY_PROJECT")
        return cb(ApiHelpers::badRequest(
            "type must be: NOT_APPLICABLE, FALSE_FINDING, MITIGATED_BY_PROJECT"));

    std::string description = (*body).get("description", "").asString();

    auto db = Database::client();
    *db << "INSERT INTO report_findings_mitigations(finding_id, type, description) "
           "VALUES($1, $2, $3) RETURNING id, finding_id, type, description, mitigated_at"
        << finding_id << type << description
        >> [cb, finding_id, db](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::serverError("insert failed"));
               int mitigation_id = r[0]["id"].as<int>();
               // Link the mitigation to the finding
               *db << "UPDATE report_findings SET mitigation_id=$1 WHERE id=$2"
                   << mitigation_id << finding_id
                   >> [cb, r](const drogon::orm::Result&) {
                          cb(ApiHelpers::created(mitigationToJson(r[0])));
                      }
                   >> [cb](const drogon::orm::DrogonDbException& e) {
                          cb(ApiHelpers::dbError(e));
                      };
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void MitigationController::get(const drogon::HttpRequestPtr&,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 int finding_id)
{
    auto db = Database::client();
    *db << "SELECT m.id, m.finding_id, m.type, m.description, m.mitigated_at "
           "FROM report_findings_mitigations m "
           "JOIN report_findings rf ON rf.mitigation_id = m.id "
           "WHERE rf.id=$1"
        << finding_id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("mitigation"));
               cb(ApiHelpers::ok(mitigationToJson(r[0])));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void MitigationController::remove(const drogon::HttpRequestPtr&,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                    int finding_id)
{
    auto db = Database::client();
    // Unlink the mitigation from this finding
    *db << "UPDATE report_findings SET mitigation_id=NULL WHERE id=$1 RETURNING mitigation_id"
        << finding_id
        >> [cb, db](const drogon::orm::Result& r) {
               if (r.empty() || r[0][0].isNull())
                   return cb(ApiHelpers::notFound("mitigation"));
               int mid = r[0][0].as<int>();
               // Only delete the mitigation record if no other findings reference it
               *db << "DELETE FROM report_findings_mitigations WHERE id=$1 "
                      "AND NOT EXISTS (SELECT 1 FROM report_findings WHERE mitigation_id=$1)"
                   << mid
                   >> [cb](const drogon::orm::Result&) { cb(ApiHelpers::noContent()); }
                   >> [cb](const drogon::orm::DrogonDbException& e) { cb(ApiHelpers::dbError(e)); };
           }
        >> [cb](const drogon::orm::DrogonDbException& e) { cb(ApiHelpers::dbError(e)); };
}

void MitigationController::createBulk(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& cb)
{
    auto body = req->getJsonObject();
    if (!body || !(*body).isMember("type") || !(*body).isMember("finding_ids"))
        return cb(ApiHelpers::badRequest("type and finding_ids required"));

    std::string type = (*body)["type"].asString();
    if (type != "NOT_APPLICABLE" && type != "FALSE_FINDING" && type != "MITIGATED_BY_PROJECT")
        return cb(ApiHelpers::badRequest(
            "type must be: NOT_APPLICABLE, FALSE_FINDING, MITIGATED_BY_PROJECT"));

    const auto& ids = (*body)["finding_ids"];
    if (!ids.isArray() || ids.empty())
        return cb(ApiHelpers::badRequest("finding_ids must be a non-empty array"));

    std::string description = (*body).get("description", "").asString();

    auto db = Database::client();
    // Insert one mitigation record
    *db << "INSERT INTO report_findings_mitigations(finding_id, type, description) "
           "VALUES($1, $2, $3) RETURNING id, finding_id, type, description, mitigated_at"
        << ids[0].asInt() << type << description
        >> [cb, db, ids](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::serverError("insert failed"));
               int mitigation_id = r[0]["id"].as<int>();

               // Link the mitigation to all specified findings
               std::vector<int> finding_ids;
               for (const auto& id : ids) finding_ids.push_back(id.asInt());

               auto completed = std::make_shared<int>(0);
               int total = static_cast<int>(finding_ids.size());
               auto result = std::make_shared<Json::Value>(mitigationToJson(r[0]));

               for (int fid : finding_ids) {
                   *db << "UPDATE report_findings SET mitigation_id=$1 WHERE id=$2"
                       << mitigation_id << fid
                       >> [cb, result, completed, total](const drogon::orm::Result&) {
                              if (++(*completed) == total)
                                  cb(ApiHelpers::created(*result));
                          }
                       >> [cb](const drogon::orm::DrogonDbException& e) {
                              cb(ApiHelpers::dbError(e));
                          };
               }
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

} // namespace blueduck
