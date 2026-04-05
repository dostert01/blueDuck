#pragma once
#include <drogon/HttpController.h>

namespace blueduck {

class CpeMappingController : public drogon::HttpController<CpeMappingController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CpeMappingController::list,
                      "/api/cpe-mappings", drogon::Get);
        ADD_METHOD_TO(CpeMappingController::create,
                      "/api/cpe-mappings", drogon::Post);
        ADD_METHOD_TO(CpeMappingController::update,
                      "/api/cpe-mappings/{id}", drogon::Put);
        ADD_METHOD_TO(CpeMappingController::remove,
                      "/api/cpe-mappings/{id}", drogon::Delete);
    METHOD_LIST_END

    void list  (const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);
    void update(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                int id);
    void remove(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                int id);
};

} // namespace blueduck
