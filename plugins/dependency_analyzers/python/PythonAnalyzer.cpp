#include "PythonAnalyzer.h"
#include <interfaces/PluginMeta.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace blueduck::plugins::python {

bool PythonAnalyzer::canAnalyze(const std::string& repo_path) const {
    return fs::exists(repo_path + "/requirements.txt") ||
           fs::exists(repo_path + "/pyproject.toml")   ||
           fs::exists(repo_path + "/setup.cfg")        ||
           fs::exists(repo_path + "/setup.py");
}

AnalysisResult PythonAnalyzer::analyze(const std::string& repo_path) {
    AnalysisResult result;

    // requirements*.txt files
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                repo_path, fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            const auto& fn = entry.path().filename().string();
            if (fn.rfind("requirements", 0) == 0 && entry.path().extension() == ".txt")
                parseRequirementsTxt(entry.path().string(), result.dependencies);
        }
    } catch (...) {}

    if (fs::exists(repo_path + "/pyproject.toml"))
        parsePyprojectToml(repo_path + "/pyproject.toml", result.dependencies);

    if (fs::exists(repo_path + "/setup.cfg"))
        parseSetupCfg(repo_path + "/setup.cfg", result.dependencies);

    result.success = true;
    return result;
}

Dependency PythonAnalyzer::depFromLine(const std::string& raw) {
    // Strip comments and whitespace
    std::string line = raw;
    auto hash = line.find('#');
    if (hash != std::string::npos) line = line.substr(0, hash);
    while (!line.empty() && (line.back() == ' ' || line.back() == '\r' || line.back() == '\n'))
        line.pop_back();

    Dependency d;
    d.ecosystem = "pypi";

    // PEP 508: name[extras] op version ; marker
    // Extract name (up to first [, ;, =, <, >, ~, !)
    static const std::regex name_re(R"(^([A-Za-z0-9]([A-Za-z0-9._\-]*[A-Za-z0-9])?))");
    static const std::regex ver_re(R"([=~!<>]+\s*([0-9][^\s,;\[]*))");

    std::smatch m;
    if (std::regex_search(line, m, name_re)) {
        d.package_name = m[1].str();
        // Normalize: lowercase, underscores
        std::transform(d.package_name.begin(), d.package_name.end(),
                        d.package_name.begin(), ::tolower);
        std::replace(d.package_name.begin(), d.package_name.end(), '-', '_');
    }

    if (std::regex_search(line, m, ver_re))
        d.package_version = m[1].str();

    return d;
}

void PythonAnalyzer::parseRequirementsTxt(const std::string& path,
                                           std::vector<Dependency>& deps) const
{
    std::ifstream f(path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '-') continue;
        auto d = depFromLine(line);
        if (!d.package_name.empty()) deps.push_back(std::move(d));
    }
}

void PythonAnalyzer::parsePyprojectToml(const std::string& path,
                                         std::vector<Dependency>& deps) const
{
    std::ifstream f(path);
    if (!f) return;

    // Minimal: look for [project] section, then "dependencies = [...]"
    static const std::regex dep_list_re(
        R"(\bdependencies\s*=\s*\[([\s\S]*?)\])", std::regex::multiline);
    static const std::regex quoted(R"re("([^"]+)")re");

    std::string content((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

    std::smatch list_m;
    if (std::regex_search(content, list_m, dep_list_re)) {
        std::string list_content = list_m[1].str();
        auto it = std::sregex_iterator(list_content.begin(), list_content.end(), quoted);
        for (; it != std::sregex_iterator(); ++it) {
            auto d = depFromLine((*it)[1].str());
            if (!d.package_name.empty()) deps.push_back(std::move(d));
        }
    }
}

void PythonAnalyzer::parseSetupCfg(const std::string& path,
                                    std::vector<Dependency>& deps) const
{
    std::ifstream f(path);
    if (!f) return;

    // Look for install_requires section (multi-line value)
    bool in_install = false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("install_requires", 0) == 0) {
            in_install = true; continue;
        }
        if (in_install) {
            if (!line.empty() && line[0] != ' ' && line[0] != '\t') {
                in_install = false; continue;
            }
            auto d = depFromLine(line);
            if (!d.package_name.empty()) deps.push_back(std::move(d));
        }
    }
}

} // namespace blueduck::plugins::python

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "dependency_analyzer", "python", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::python::PythonAnalyzer();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::python::PythonAnalyzer*>(p);
}

} // extern "C"
