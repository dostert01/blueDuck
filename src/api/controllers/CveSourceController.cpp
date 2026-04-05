#include "CveSourceController.h"
#include <api/ApiHelpers.h>
#include <core/Application.h>
#include <drogon/drogon.h>

namespace blueduck {

void CveSourceController::list(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb)
{
    const auto& sources = Application::instance().cveSources();
    Json::Value arr(Json::arrayValue);
    for (const auto& [name, src] : sources) {
        Json::Value o;
        o["name"] = name;
        arr.append(o);
    }
    cb(ApiHelpers::ok(arr));
}

void CveSourceController::sync(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                std::string name)
{
    const auto& sources = Application::instance().cveSources();
    if (sources.find(name) == sources.end())
        return cb(ApiHelpers::notFound("cve source '" + name + "'"));

    drogon::async_run([name]() -> drogon::Task<> {
        Application::instance().syncCveSource(name);
        co_return;
    });

    Json::Value resp; resp["status"] = "sync started";
    cb(ApiHelpers::accepted(resp));
}

} // namespace blueduck
