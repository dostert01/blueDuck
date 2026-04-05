#include "NpmAnalyzer.h"
#include <interfaces/PluginMeta.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace blueduck::plugins::npm {

bool NpmAnalyzer::canAnalyze(const std::string& repo_path) const {
    return fs::exists(repo_path + "/package.json");
}

AnalysisResult NpmAnalyzer::analyze(const std::string& repo_path) {
    AnalysisResult result;

    // Collect all package.json files (root + workspaces)
    std::vector<std::string> pkg_files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                repo_path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file() && entry.path().filename() == "package.json") {
                // Skip node_modules
                auto rel = fs::relative(entry.path(), repo_path).string();
                if (rel.find("node_modules") != std::string::npos) continue;
                pkg_files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        result.error_message = e.what();
        return result;
    }

    // Regex to strip semver range operators: ^, ~, >=, <=, >, <, =
    static const std::regex semver_prefix(R"(^[\^~>=<v\s]+)");

    auto add_deps = [&](const json& node, const std::string& key) {
        if (!node.contains(key)) return;
        for (const auto& [name, ver_node] : node[key].items()) {
            Dependency d;
            d.ecosystem    = "npm";
            d.package_name = name;

            std::string ver = ver_node.is_string() ? ver_node.get<std::string>() : "";
            // Strip range operators to get a plain version
            d.package_version = std::regex_replace(ver, semver_prefix, "");
            result.dependencies.push_back(std::move(d));
        }
    };

    for (const auto& pf : pkg_files) {
        std::ifstream f(pf);
        if (!f) continue;
        json pkg;
        try { pkg = json::parse(f); }
        catch (...) { continue; }

        add_deps(pkg, "dependencies");
        add_deps(pkg, "devDependencies");
        add_deps(pkg, "peerDependencies");
        add_deps(pkg, "optionalDependencies");
    }

    result.success = true;
    return result;
}

} // namespace blueduck::plugins::npm

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "dependency_analyzer", "npm", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::npm::NpmAnalyzer();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::npm::NpmAnalyzer*>(p);
}

} // extern "C"
