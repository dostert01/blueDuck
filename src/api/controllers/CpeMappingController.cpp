#include "CpeMappingController.h"
#include <api/ApiHelpers.h>
#include <db/Database.h>
#include <db/FieldHelpers.h>

namespace blueduck {

namespace {

Json::Value rowToJson(const drogon::orm::Row& row) {
    Json::Value o;
    o["id"]              = row["id"].as<int>();
    o["ecosystem"]       = row["ecosystem"].as<std::string>();
    o["package_name"]    = row["package_name"].as<std::string>();
    o["cpe_vendor"]      = row["cpe_vendor"].as<std::string>();
    o["cpe_product"]     = row["cpe_product"].as<std::string>();
    o["git_url_pattern"] = fieldOrEmpty(row["git_url_pattern"]);
    return o;
}

} // anonymous namespace

void CpeMappingController::list(const drogon::HttpRequestPtr&,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb)
{
    auto db = Database::client();
    *db << "SELECT id, ecosystem, package_name, cpe_vendor, cpe_product, "
           "git_url_pattern FROM ecosystem_cpe_mapping "
           "ORDER BY ecosystem, package_name"
        >> [cb](const drogon::orm::Result& r) {
               Json::Value arr(Json::arrayValue);
               for (const auto& row : r)
                   arr.append(rowToJson(row));
               cb(ApiHelpers::ok(arr));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void CpeMappingController::create(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& cb)
{
    auto body = req->getJsonObject();
    if (!body ||
        !(*body).isMember("ecosystem") ||
        !(*body).isMember("package_name") ||
        !(*body).isMember("cpe_vendor") ||
        !(*body).isMember("cpe_product"))
        return cb(ApiHelpers::badRequest(
            "ecosystem, package_name, cpe_vendor, cpe_product required"));

    std::string ecosystem       = (*body)["ecosystem"].asString();
    std::string package_name    = (*body)["package_name"].asString();
    std::string cpe_vendor      = (*body)["cpe_vendor"].asString();
    std::string cpe_product     = (*body)["cpe_product"].asString();
    std::string git_url_pattern = (*body).get("git_url_pattern", "").asString();

    auto db = Database::client();
    if (git_url_pattern.empty()) {
        *db << "INSERT INTO ecosystem_cpe_mapping"
               "(ecosystem, package_name, cpe_vendor, cpe_product) "
               "VALUES($1,$2,$3,$4) "
               "RETURNING id, ecosystem, package_name, cpe_vendor, cpe_product, git_url_pattern"
            << ecosystem << package_name << cpe_vendor << cpe_product
            >> [cb](const drogon::orm::Result& r) {
                   cb(ApiHelpers::created(rowToJson(r[0])));
               }
            >> [cb](const drogon::orm::DrogonDbException& e) {
                   cb(ApiHelpers::dbError(e));
               };
    } else {
        *db << "INSERT INTO ecosystem_cpe_mapping"
               "(ecosystem, package_name, cpe_vendor, cpe_product, git_url_pattern) "
               "VALUES($1,$2,$3,$4,$5) "
               "RETURNING id, ecosystem, package_name, cpe_vendor, cpe_product, git_url_pattern"
            << ecosystem << package_name << cpe_vendor << cpe_product << git_url_pattern
            >> [cb](const drogon::orm::Result& r) {
                   cb(ApiHelpers::created(rowToJson(r[0])));
               }
            >> [cb](const drogon::orm::DrogonDbException& e) {
                   cb(ApiHelpers::dbError(e));
               };
    }
}

void CpeMappingController::update(const drogon::HttpRequestPtr& req,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                    int id)
{
    auto body = req->getJsonObject();
    if (!body ||
        !(*body).isMember("ecosystem") ||
        !(*body).isMember("package_name") ||
        !(*body).isMember("cpe_vendor") ||
        !(*body).isMember("cpe_product"))
        return cb(ApiHelpers::badRequest(
            "ecosystem, package_name, cpe_vendor, cpe_product required"));

    std::string ecosystem       = (*body)["ecosystem"].asString();
    std::string package_name    = (*body)["package_name"].asString();
    std::string cpe_vendor      = (*body)["cpe_vendor"].asString();
    std::string cpe_product     = (*body)["cpe_product"].asString();
    std::string git_url_pattern = (*body).get("git_url_pattern", "").asString();

    auto db = Database::client();
    *db << "UPDATE ecosystem_cpe_mapping SET "
           "ecosystem=$2, package_name=$3, cpe_vendor=$4, cpe_product=$5, "
           "git_url_pattern=NULLIF($6,'') "
           "WHERE id=$1 "
           "RETURNING id, ecosystem, package_name, cpe_vendor, cpe_product, git_url_pattern"
        << id << ecosystem << package_name << cpe_vendor << cpe_product << git_url_pattern
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("cpe mapping"));
               cb(ApiHelpers::ok(rowToJson(r[0])));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void CpeMappingController::remove(const drogon::HttpRequestPtr&,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                    int id)
{
    auto db = Database::client();
    *db << "DELETE FROM ecosystem_cpe_mapping WHERE id=$1 RETURNING id" << id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("cpe mapping"));
               cb(ApiHelpers::noContent());
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

} // namespace blueduck
