#pragma once
#include <string>
#include <filesystem>

namespace blueduck {

class Migrator {
public:
    explicit Migrator(std::string connstr,
                      std::filesystem::path migrations_dir);

    void migrate();

private:
    void ensureMigrationsTable();
    int  currentVersion();
    void applyFile(int version, const std::filesystem::path& path);

    std::string           connstr_;
    std::filesystem::path migrations_dir_;
};

} // namespace blueduck
