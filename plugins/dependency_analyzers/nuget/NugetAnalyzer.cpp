#include "NugetAnalyzer.h"
#include <interfaces/PluginMeta.h>
#include <pugixml.hpp>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace blueduck::plugins::nuget {

static const std::vector<std::string> PROJECT_EXTS = {".csproj", ".fsproj", ".vbproj"};

bool NugetAnalyzer::canAnalyze(const std::string& repo_path) const {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                repo_path, fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            const auto& ext = entry.path().extension().string();
            for (const auto& e : PROJECT_EXTS)
                if (ext == e) return true;
            if (entry.path().filename() == "packages.config") return true;
        }
    } catch (...) {}
    return false;
}

AnalysisResult NugetAnalyzer::analyze(const std::string& repo_path) {
    AnalysisResult result;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                repo_path, fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            const auto& p = entry.path();
            const auto& ext = p.extension().string();

            for (const auto& e : PROJECT_EXTS) {
                if (ext == e) {
                    parseProjectFile(p.string(), result.dependencies);
                    break;
                }
            }
            if (p.filename() == "packages.config")
                parsePackagesConfig(p.string(), result.dependencies);
        }
    } catch (const std::exception& e) {
        result.error_message = e.what();
        return result;
    }

    result.success = true;
    return result;
}

void NugetAnalyzer::parseProjectFile(const std::string& path,
                                      std::vector<Dependency>& deps) const
{
    pugi::xml_document doc;
    if (!doc.load_file(path.c_str())) return;

    // SDK-style: <PackageReference Include="Name" Version="x.y.z" />
    // or:        <PackageReference Include="Name"><Version>x.y.z</Version></PackageReference>
    for (const auto& node : doc.select_nodes("//PackageReference")) {
        Dependency d;
        d.ecosystem    = "nuget";
        d.package_name = node.node().attribute("Include").as_string();

        // Normalize: lowercase
        std::transform(d.package_name.begin(), d.package_name.end(),
                        d.package_name.begin(), ::tolower);

        std::string ver = node.node().attribute("Version").as_string();
        if (ver.empty()) {
            auto ver_child = node.node().child("Version");
            if (ver_child) ver = ver_child.text().as_string();
        }
        d.package_version = ver;

        if (!d.package_name.empty()) deps.push_back(std::move(d));
    }
}

void NugetAnalyzer::parsePackagesConfig(const std::string& path,
                                         std::vector<Dependency>& deps) const
{
    pugi::xml_document doc;
    if (!doc.load_file(path.c_str())) return;

    // <package id="Name" version="x.y.z" ... />
    for (const auto& node : doc.select_nodes("//package")) {
        Dependency d;
        d.ecosystem    = "nuget";
        d.package_name = node.node().attribute("id").as_string();
        std::transform(d.package_name.begin(), d.package_name.end(),
                        d.package_name.begin(), ::tolower);
        d.package_version = node.node().attribute("version").as_string();
        if (!d.package_name.empty()) deps.push_back(std::move(d));
    }
}

} // namespace blueduck::plugins::nuget

namespace {
    static blueduck::PluginMeta _bd_meta = {
        BLUEDUCK_PLUGIN_API_VERSION, "dependency_analyzer", "nuget", "1.0.0"
    };
}

extern "C" {

const blueduck::PluginMeta* blueduck_plugin_meta() { return &_bd_meta; }

void* blueduck_create_plugin() {
    return new blueduck::plugins::nuget::NugetAnalyzer();
}

void blueduck_destroy_plugin(void* p) {
    delete static_cast<blueduck::plugins::nuget::NugetAnalyzer*>(p);
}

} // extern "C"
