#pragma once
#include <db/Database.h>
#include <string>
#include <map>

namespace blueduck {

struct ServerConfig {
    uint16_t    port          = 8080;
    int         threads       = 0;
    std::string document_root;
};

struct Config {
    ServerConfig                       server;
    DbConfig                           db;
    std::string                        plugin_dir;
    std::string                        repos_dir;
    std::string                        migrations_dir;
    std::map<std::string, std::string> plugin_config;
};

Config loadConfig(const std::string& path);

} // namespace blueduck
