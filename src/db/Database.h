#pragma once
#include <drogon/drogon.h>
#include <string>
#include <cstdint>

namespace blueduck {

struct DbConfig {
    std::string host      = "localhost";
    uint16_t    port      = 5432;
    std::string name;
    std::string user;
    std::string password;
    uint32_t    pool_size = 10;
};

class Database {
public:
    static void configure(const DbConfig& config);
    static drogon::orm::DbClientPtr client();
    static drogon::orm::DbClientPtr blockingClient();
    static const std::string& connectionString();
private:
    static drogon::orm::DbClientPtr blocking_client_;
    static std::string connstr_;
};

} // namespace blueduck
