#pragma once
#include <drogon/HttpController.h>

namespace blueduck {

class CveSourceController : public drogon::HttpController<CveSourceController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CveSourceController::list, "/api/cve-sources",          drogon::Get);
        ADD_METHOD_TO(CveSourceController::sync, "/api/cve-sources/{name}/sync", drogon::Post);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&);
    void sync(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&,
              std::string name);
};

} // namespace blueduck
