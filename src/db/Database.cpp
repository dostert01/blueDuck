#include "Database.h"
#include <format>

namespace blueduck {

drogon::orm::DbClientPtr Database::blocking_client_;
std::string Database::connstr_;

void Database::configure(const DbConfig& cfg) {
    // Async connection pool — used by all request handlers (created inside Drogon,
    // available once app().run() starts the event loop)
    drogon::app().createDbClient(
        "postgresql",
        cfg.host, cfg.port, cfg.name, cfg.user, cfg.password,
        cfg.pool_size,
        /*filename=*/"",
        /*name=*/"default"
    );

    // Standalone blocking client — usable immediately (before app().run()),
    // needed for migrations at startup and for AnalysisService.
    connstr_ = std::format(
        "host={} port={} dbname={} user={} password='{}'",
        cfg.host, cfg.port, cfg.name, cfg.user, cfg.password);

    blocking_client_ = drogon::orm::DbClient::newPgClient(connstr_, 1, true);
}

drogon::orm::DbClientPtr Database::client() {
    return drogon::app().getDbClient("default");
}

drogon::orm::DbClientPtr Database::blockingClient() {
    return blocking_client_;
}

const std::string& Database::connectionString() {
    return connstr_;
}

} // namespace blueduck
