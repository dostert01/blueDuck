#include "CmakeAnalyzer.h"
#include <interfaces/PluginMeta.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace blueduck::plugins::cmake_cpp {

bool CmakeAnalyzer::canAnalyze(const std::string& repo_path) const {
    return fs::exists(repo_path + "/CMakeLists.txt");
}

AnalysisResult CmakeAnalyzer::analyze(const std::string& repo_path) {
    AnalysisResult result;
    if (!canAnalyze(repo_path)) {
        result.error_message = "no CMakeLists.txt found";
        return result;
    }

    scanDirectory(repo_path, result.dependencies);

    // Deduplicate by name
    std::sort(result.dependencies.begin(), result.dependencies.end(),
        [](const Dependency& a, const Dependency& b) {
            return a.package_name < b.package_name;
        });
    result.dependencies.erase(
        std::unique(result.dependencies.begin(), result.dependencies.end(),
            [](const Dependency& a, const Dependency& b) {
                return a.package_name == b.package_name;
            }),
        result.dependencies.end());

    result.success = true;
    return result;
}

void CmakeAnalyzer::scanDirectory(const std::string& dir,
                                   std::vector<Dependency>& deps) const
{
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                 dir, fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            const auto& p = entry.path();
            if (p.filename() == "CMakeLists.txt" ||
                (p.extension() == ".cmake" && p.filename() != "CMakeLists.txt"))
            {
                parseFile(p.string(), deps);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[CmakeAnalyzer] scanDirectory error: " << e.what() << "\n";
    }
}

void CmakeAnalyzer::parseFile(const std::string& file_path,
                               std::vector<Dependency>& deps) const
{
    std::ifstream f(file_path);
    if (!f) return;

    std::string content((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

    // find_package(Name [version] ...)
    // Captures: group 1 = package name, group 2 (optional) = version
    static const std::regex find_pkg(
        R"(\bfind_package\s*\(\s*([A-Za-z0-9_\-]+)\s*([0-9][0-9.]*)?)",
        std::regex::icase);

    // FetchContent_Declare(name GIT_REPOSITORY ... GIT_TAG tag)
    static const std::regex fetch_decl(
        R"(\bFetchContent_Declare\s*\(\s*([A-Za-z0-9_\-]+))",
        std::regex::icase);
    static const std::regex git_tag(
        R"(\bGIT_TAG\s+([^\s\)]+))",
        std::regex::icase);

    // find_package matches
    auto begin = std::sregex_iterator(content.begin(), content.end(), find_pkg);
    for (auto it = begin; it != std::sregex_iterator(); ++it) {
        Dependency d;
        d.ecosystem    = "cmake";
        d.package_name = (*it)[1].str();
        if ((*it)[2].matched)
            d.package_version = (*it)[2].str();
        deps.push_back(std::move(d));
    }

    // FetchContent_Declare matches — try to extract GIT_TAG as version
    auto fc_begin = std::sregex_iterator(content.begin(), content.end(), fetch_decl);
    for (auto it = fc_begin; it != std::sregex_iterator(); ++it) {
        Dependency d;
        d.ecosystem    = "cmake";
        d.package_name = (*it)[1].str();

        // Search for GIT_TAG in the next ~300 chars after the match
        size_t start = static_cast<size_t>((*it).position()) + (*it).length();
        size_t end   = std::min(start + 300, content.size());
        std::string snippet = content.substr(start, end - start);

        std::smatch tag_match;
        if (std::regex_search(snippet, tag_match, git_tag))
            d.package_version = tag_match[1].str();

        deps.push_back(std::move(d));
    }
}

} // namespace blueduck::plugins::cmake_cpp

// ---------- Plugin entry points ----------

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "dependency_analyzer", "cmake_cpp", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::cmake_cpp::CmakeAnalyzer();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::cmake_cpp::CmakeAnalyzer*>(p);
}

} // extern "C"
