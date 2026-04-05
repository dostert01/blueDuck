#pragma once
#include <drogon/HttpController.h>

namespace blueduck {

class VersionController : public drogon::HttpController<VersionController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(VersionController::list,     "/api/projects/{project_id}/versions",       drogon::Get);
        ADD_METHOD_TO(VersionController::get,      "/api/projects/{project_id}/versions/{id}",  drogon::Get);
        ADD_METHOD_TO(VersionController::remove,   "/api/projects/{project_id}/versions/{id}",  drogon::Delete);
        ADD_METHOD_TO(VersionController::analyze,  "/api/projects/{project_id}/versions/{id}/analyze", drogon::Post);
    METHOD_LIST_END

    void list   (const drogon::HttpRequestPtr&,
                 std::function<void(const drogon::HttpResponsePtr&)>&&,
                 int project_id);
    void get    (const drogon::HttpRequestPtr&,
                 std::function<void(const drogon::HttpResponsePtr&)>&&,
                 int project_id, int id);
    void remove (const drogon::HttpRequestPtr&,
                 std::function<void(const drogon::HttpResponsePtr&)>&&,
                 int project_id, int id);
    void analyze(const drogon::HttpRequestPtr&,
                 std::function<void(const drogon::HttpResponsePtr&)>&&,
                 int project_id, int id);
};

} // namespace blueduck
