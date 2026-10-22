#pragma once
#include <drogon/HttpController.h>

namespace blueduck {

class MitigationController : public drogon::HttpController<MitigationController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(MitigationController::create,
                      "/api/findings/{finding_id}/mitigation", drogon::Post);
        ADD_METHOD_TO(MitigationController::get,
                      "/api/findings/{finding_id}/mitigation", drogon::Get);
        ADD_METHOD_TO(MitigationController::remove,
                      "/api/findings/{finding_id}/mitigation", drogon::Delete);
        ADD_METHOD_TO(MitigationController::createBulk,
                      "/api/findings/mitigate-bulk", drogon::Post);
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                int finding_id);
    void get   (const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                int finding_id);
    void remove(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                int finding_id);
    void createBulk(const drogon::HttpRequestPtr&,
                    std::function<void(const drogon::HttpResponsePtr&)>&&);
};

} // namespace blueduck
