#include "CargoAnalyzer.h"
#include <interfaces/PluginMeta.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

namespace blueduck::plugins::cargo {

bool CargoAnalyzer::canAnalyze(const std::string& repo_path) const {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                repo_path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file() && entry.path().filename() == "Cargo.toml")
                return true;
        }
    } catch (...) {}
    return false;
}

AnalysisResult CargoAnalyzer::analyze(const std::string& repo_path) {
    AnalysisResult result;

    // Collect all Cargo.toml files (workspace members too)
    std::vector<std::string> toml_files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                repo_path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file() && entry.path().filename() == "Cargo.toml")
                toml_files.push_back(entry.path().string());
        }
    } catch (const std::exception& e) {
        result.error_message = e.what();
        return result;
    }

    // Minimal TOML parser for [dependencies] / [dev-dependencies] sections.
    // Handles:
    //   name = "1.2.3"
    //   name = { version = "1.2.3", ... }
    //   name.version = "1.2.3"
    static const std::regex section_re(R"(^\s*\[([\w\-\.]+)\]\s*$)");
    static const std::regex simple_dep_re(
        R"re(^\s*([A-Za-z0-9_\-]+)\s*=\s*"([^"]+)")re");
    static const std::regex table_dep_re(
        R"re(^\s*([A-Za-z0-9_\-]+)\s*=\s*\{[^}]*version\s*=\s*"([^"]+)")re");
    static const std::regex dotted_dep_re(
        R"re(^\s*([A-Za-z0-9_\-]+)\.version\s*=\s*"([^"]+)")re");
    // Strip semver range operators
    static const std::regex ver_prefix(R"(^[\^~>=<\s]+)");

    auto is_dep_section = [](const std::string& s) {
        return s == "dependencies"     || s == "dev-dependencies"  ||
               s == "build-dependencies";
    };

    for (const auto& tf : toml_files) {
        std::ifstream f(tf);
        if (!f) continue;

        std::string line;
        std::string current_section;
        while (std::getline(f, line)) {
            // Strip inline comments
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);

            std::smatch m;
            if (std::regex_match(line, m, section_re)) {
                current_section = m[1].str();
                continue;
            }

            if (!is_dep_section(current_section)) continue;

            auto try_add = [&](const std::smatch& dm) {
                Dependency d;
                d.ecosystem    = "cargo";
                d.package_name = dm[1].str();
                std::string ver = dm[2].str();
                d.package_version = std::regex_replace(ver, ver_prefix, "");
                result.dependencies.push_back(std::move(d));
            };

            if (std::regex_search(line, m, table_dep_re))  { try_add(m); continue; }
            if (std::regex_search(line, m, dotted_dep_re)) { try_add(m); continue; }
            if (std::regex_search(line, m, simple_dep_re)) { try_add(m); continue; }
        }
    }

    result.success = true;
    return result;
}

} // namespace blueduck::plugins::cargo

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "dependency_analyzer", "cargo", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::cargo::CargoAnalyzer();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::cargo::CargoAnalyzer*>(p);
}

} // extern "C"
