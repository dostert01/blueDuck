#pragma once
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>
#include <json/json.h>

namespace blueduck::ApiHelpers {

using Resp = drogon::HttpResponsePtr;

inline Resp ok(const Json::Value& body) {
    auto r = drogon::HttpResponse::newHttpJsonResponse(body);
    r->setStatusCode(drogon::k200OK);
    return r;
}

inline Resp created(const Json::Value& body) {
    auto r = drogon::HttpResponse::newHttpJsonResponse(body);
    r->setStatusCode(drogon::k201Created);
    return r;
}

inline Resp accepted(const Json::Value& body) {
    auto r = drogon::HttpResponse::newHttpJsonResponse(body);
    r->setStatusCode(drogon::k202Accepted);
    return r;
}

inline Resp noContent() {
    return drogon::HttpResponse::newHttpResponse(drogon::k204NoContent, drogon::CT_NONE);
}

inline Resp badRequest(const std::string& msg) {
    Json::Value e; e["error"] = msg;
    auto r = drogon::HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(drogon::k400BadRequest);
    return r;
}

inline Resp notFound(const std::string& resource) {
    Json::Value e; e["error"] = resource + " not found";
    auto r = drogon::HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(drogon::k404NotFound);
    return r;
}

inline Resp serverError(const std::string& msg) {
    Json::Value e; e["error"] = msg;
    auto r = drogon::HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(drogon::k500InternalServerError);
    return r;
}

inline Resp dbError(const drogon::orm::DrogonDbException& e) {
    return serverError(e.base().what());
}

inline Json::Value projectRowToJson(const drogon::orm::Row& row) {
    Json::Value o;
    o["id"]             = row["id"].as<int>();
    o["name"]           = row["name"].as<std::string>();
    o["git_url"]        = row["git_url"].as<std::string>();
    o["description"]    = row["description"].isNull() ? std::string("") : row["description"].as<std::string>();
    o["created_at"]     = row["created_at"].as<std::string>();
    o["last_synced_at"] = row["last_synced_at"].isNull()
                          ? Json::Value()
                          : Json::Value(row["last_synced_at"].as<std::string>());
    return o;
}

} // namespace blueduck::ApiHelpers
