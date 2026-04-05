#include <gtest/gtest.h>
#include <db/Database.h>
#include <db/Migrator.h>
#include <drogon/drogon.h>
#include <cstdlib>

using namespace blueduck;

/// Integration test — requires a real PostgreSQL instance.
/// Set env var BD_TEST_DB_URL to a connection string, e.g.:
///   postgresql://blueduck:secret@localhost:5432/blueduck_test
class MigratorIntegrationTest : public ::testing::Test {
protected:
    static std::string connstr() {
        const char* env = std::getenv("BD_TEST_DB_URL");
        return env ? env : "";
    }

    void SetUp() override {
        if (connstr().empty())
            GTEST_SKIP() << "BD_TEST_DB_URL not set; skipping integration test";
    }
};

TEST_F(MigratorIntegrationTest, RunsMigrationsIdempotently) {
    Migrator m(connstr(), "db/migrations");
    EXPECT_NO_THROW(m.migrate());
    EXPECT_NO_THROW(m.migrate()); // idempotent
}

TEST_F(MigratorIntegrationTest, TablesExistAfterMigration) {
    Migrator m(connstr(), "db/migrations");
    m.migrate();

    DbConfig cfg;
    cfg.name = connstr();
    cfg.pool_size = 1;
    Database::configure(cfg);

    auto db = Database::blockingClient();
    auto r = db->execSqlSync(
        "SELECT COUNT(*) FROM information_schema.tables "
        "WHERE table_schema='public' AND table_name IN "
        "('projects','cve_records','reports','report_findings')");
    EXPECT_EQ(r[0][0].as<int>(), 4);
}
