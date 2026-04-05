#include "Migrator.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <format>
#include <iostream>
#include <libpq-fe.h>

namespace blueduck {

Migrator::Migrator(std::string connstr,
                   std::filesystem::path migrations_dir)
    : connstr_(std::move(connstr))
    , migrations_dir_(std::move(migrations_dir))
{}

namespace {
    // RAII wrapper for PGconn
    struct PgConn {
        PGconn* conn;
        explicit PgConn(const std::string& connstr) {
            conn = PQconnectdb(connstr.c_str());
            if (PQstatus(conn) != CONNECTION_OK)
                throw std::runtime_error(
                    std::format("Migrator DB connect failed: {}", PQerrorMessage(conn)));
        }
        ~PgConn() { if (conn) PQfinish(conn); }
        PgConn(const PgConn&) = delete;
        PgConn& operator=(const PgConn&) = delete;

        void exec(const std::string& sql) {
            PGresult* res = PQexec(conn, sql.c_str());
            ExecStatusType st = PQresultStatus(res);
            if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
                std::string err = PQresultErrorMessage(res);
                PQclear(res);
                throw std::runtime_error(err);
            }
            PQclear(res);
        }

        std::string queryScalar(const std::string& sql) {
            PGresult* res = PQexec(conn, sql.c_str());
            if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                std::string err = PQresultErrorMessage(res);
                PQclear(res);
                throw std::runtime_error(err);
            }
            std::string val;
            if (PQntuples(res) > 0 && PQnfields(res) > 0)
                val = PQgetvalue(res, 0, 0);
            PQclear(res);
            return val;
        }
    };
}

void Migrator::migrate() {
    PgConn pg(connstr_);

    ensureMigrationsTable();

    std::regex pattern(R"(^(\d{3})_.+\.sql$)");
    std::vector<std::pair<int, std::filesystem::path>> pending;

    for (const auto& entry : std::filesystem::directory_iterator(migrations_dir_)) {
        std::smatch m;
        std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, m, pattern))
            pending.emplace_back(std::stoi(m[1]), entry.path());
    }

    std::sort(pending.begin(), pending.end());

    int current = currentVersion();
    for (const auto& [version, path] : pending) {
        if (version <= current) continue;
        applyFile(version, path);
        current = version;
    }

    std::cout << "[Migrator] Schema is up to date (version " << current << ")\n";
}

void Migrator::ensureMigrationsTable() {
    PgConn pg(connstr_);
    pg.exec(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version    INT PRIMARY KEY,
            filename   TEXT NOT NULL,
            applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
        )
    )");
}

int Migrator::currentVersion() {
    PgConn pg(connstr_);
    auto val = pg.queryScalar(
        "SELECT COALESCE(MAX(version), 0) FROM schema_migrations");
    return val.empty() ? 0 : std::stoi(val);
}

void Migrator::applyFile(int version, const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error(
            std::format("Cannot open migration file: {}", path.string()));

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string sql = ss.str();

    std::cout << std::format("[Migrator] Applying migration {:03d}: {}\n",
                              version, path.filename().string());

    PgConn pg(connstr_);
    try {
        pg.exec("BEGIN");
        pg.exec(sql);
        pg.exec(std::format(
            "INSERT INTO schema_migrations(version, filename) VALUES({}, '{}')",
            version, path.filename().string()));
        pg.exec("COMMIT");
    } catch (...) {
        pg.exec("ROLLBACK");
        throw std::runtime_error(
            std::format("Migration {:03d} failed", version));
    }
}

} // namespace blueduck
