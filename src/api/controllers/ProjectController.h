#pragma once
#include <drogon/HttpController.h>

namespace blueduck {

class ProjectController : public drogon::HttpController<ProjectController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ProjectController::create,         "/api/projects",                drogon::Post);
        ADD_METHOD_TO(ProjectController::list,           "/api/projects",                drogon::Get);
        ADD_METHOD_TO(ProjectController::get,            "/api/projects/{id}",           drogon::Get);
        ADD_METHOD_TO(ProjectController::remove,         "/api/projects/{id}",           drogon::Delete);
        ADD_METHOD_TO(ProjectController::setCredentials, "/api/projects/{id}/credentials", drogon::Post);
        ADD_METHOD_TO(ProjectController::sync,           "/api/projects/{id}/sync",      drogon::Post);
        ADD_METHOD_TO(ProjectController::testConnection, "/api/projects/{id}/test-connection", drogon::Post);
    METHOD_LIST_END

    void create        (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&);
    void list          (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&);
    void get           (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&, int id);
    void remove        (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&, int id);
    void setCredentials(const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&, int id);
    void sync          (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&, int id);
    void testConnection(const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&, int id);
};

} // namespace blueduck
