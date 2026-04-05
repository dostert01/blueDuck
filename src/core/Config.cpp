#include "Config.h"
#include <json/json.h>
#include <fstream>
#include <stdexcept>
#include <format>

namespace blueduck {

namespace {
    std::string getEnv(const char* name, const std::string& fallback = "") {
        const char* v = std::getenv(name);
        return v ? v : fallback;
    }

    Json::Value loadJson(const std::string& path) {
        std::ifstream f(path);
        if (!f)
            throw std::runtime_error(std::format("Cannot open config file: {}", path));

        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;
        if (!Json::parseFromStream(reader, f, &root, &errs))
            throw std::runtime_error(std::format("Config parse error: {}", errs));

        return root;
    }
}

Config loadConfig(const std::string& path) {
    Json::Value j = loadJson(path);
    Config cfg;

    const auto& srv          = j["server"];
    cfg.server.port          = srv.get("port",          8080).asUInt();
    cfg.server.threads       = srv.get("threads",       0   ).asInt();
    cfg.server.document_root = srv.get("document_root", ""  ).asString();

    const auto& db   = j["database"];
    cfg.db.host      = getEnv("BD_DB_HOST",     db.get("host",     "localhost").asString());
    cfg.db.port      = static_cast<uint16_t>(
                        std::stoi(getEnv("BD_DB_PORT",
                            std::to_string(db.get("port", 5432).asUInt()))));
    cfg.db.name      = getEnv("BD_DB_NAME",     db.get("name",     "blueduck").asString());
    cfg.db.user      = getEnv("BD_DB_USER",     db.get("user",     "blueduck").asString());
    cfg.db.password  = getEnv("BD_DB_PASSWORD", db.get("password", "").asString());
    cfg.db.pool_size = db.get("pool_size", 10).asUInt();

    cfg.plugin_dir     = j.get("plugin_dir",     "/usr/lib/blueduck/plugins").asString();
    cfg.repos_dir      = j.get("repos_dir",      "/var/lib/blueduck/repos").asString();
    cfg.migrations_dir = j.get("migrations_dir", "/usr/share/blueduck/migrations").asString();

    const auto& pc = j["plugin_config"];
    for (const auto& key : pc.getMemberNames())
        cfg.plugin_config[key] = pc[key].asString();

    cfg.plugin_config["db_connstr"] = std::format(
        "host={} port={} dbname={} user={} password='{}'",
        cfg.db.host, cfg.db.port, cfg.db.name,
        cfg.db.user, cfg.db.password);

    if (const char* k = std::getenv("BD_NVD_API_KEY"))
        cfg.plugin_config["nvd_api_key"] = k;

    return cfg;
}

} // namespace blueduck
