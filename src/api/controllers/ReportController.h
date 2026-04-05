#pragma once
#include <drogon/HttpController.h>

namespace blueduck {

class ReportController : public drogon::HttpController<ReportController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ReportController::listForVersion,
                      "/api/projects/{project_id}/versions/{version_id}/reports", drogon::Get);
        ADD_METHOD_TO(ReportController::get,
                      "/api/reports/{id}", drogon::Get);
        ADD_METHOD_TO(ReportController::findings,
                      "/api/reports/{id}/findings", drogon::Get);
        ADD_METHOD_TO(ReportController::dependencies,
                      "/api/reports/{id}/dependencies", drogon::Get);
        ADD_METHOD_TO(ReportController::generate,
                      "/api/projects/{project_id}/versions/{version_id}/reports", drogon::Post);
    METHOD_LIST_END

    void listForVersion(const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&,
                        int project_id, int version_id);
    void get           (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&,
                        int id);
    void findings      (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&,
                        int id);
    void dependencies  (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&,
                        int id);
    void generate      (const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&,
                        int project_id, int version_id);
};

} // namespace blueduck
