#include "PluginLoader.h"
#include <dlfcn.h>
#include <map>
#include <stdexcept>
#include <format>
#include <iostream>

namespace blueduck {

template<typename T>
struct PluginDeleter {
    void (*destroy_fn)(void*);
    void operator()(T* ptr) const {
        if (destroy_fn && ptr)
            destroy_fn(static_cast<void*>(ptr));
    }
};

PluginLoader::~PluginLoader() {
    cve_sources_.clear();
    analyzers_.clear();
    for (void* handle : dl_handles_)
        dlclose(handle);
}

void PluginLoader::loadDirectory(const std::filesystem::path& dir,
                                  const std::string& type_hint)
{
    if (!std::filesystem::exists(dir)) {
        std::cerr << std::format("[PluginLoader] Directory not found: {}\n", dir.string());
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir))
        if (entry.path().extension() == ".so")
            loadFile(entry.path());
}

void PluginLoader::loadFile(const std::filesystem::path& path) {
    try {
        auto raw = loadRaw(path);
        registerPlugin(std::move(raw));
    } catch (const std::exception& e) {
        std::cerr << std::format("[PluginLoader] Failed to load {}: {}\n",
                                  path.string(), e.what());
    }
}

PluginLoader::RawPlugin PluginLoader::loadRaw(const std::filesystem::path& path) {
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle)
        throw std::runtime_error(dlerror());

    auto resolve = [&](const char* symbol, void** fn) {
        dlerror();
        *fn = dlsym(handle, symbol);
        const char* err = dlerror();
        if (err) {
            dlclose(handle);
            throw std::runtime_error(
                std::format("Symbol '{}' not found: {}", symbol, err));
        }
    };

    RawPlugin raw;
    raw.dl_handle = handle;

    using MetaFn = const PluginMeta*(*)();
    MetaFn meta_fn = nullptr;
    resolve("blueduck_plugin_meta",    reinterpret_cast<void**>(&meta_fn));
    resolve("blueduck_create_plugin",  reinterpret_cast<void**>(&raw.create));
    resolve("blueduck_destroy_plugin", reinterpret_cast<void**>(&raw.destroy));

    raw.meta = meta_fn();
    if (!raw.meta)
        throw std::runtime_error("blueduck_plugin_meta() returned null");

    if (raw.meta->api_version != kSupportedApiVersion) {
        dlclose(handle);
        throw std::runtime_error(
            std::format("API version mismatch: plugin={}, loader={}",
                         raw.meta->api_version, kSupportedApiVersion));
    }

    return raw;
}

void PluginLoader::registerPlugin(RawPlugin raw) {
    void* instance_ptr = raw.create();
    if (!instance_ptr)
        throw std::runtime_error(
            std::format("blueduck_create_plugin() returned null for '{}'",
                         raw.meta->plugin_name));

    dl_handles_.push_back(raw.dl_handle);
    const std::string type(raw.meta->plugin_type);
    const std::string name(raw.meta->plugin_name);
    const std::string ver(raw.meta->plugin_version);

    if (type == "cve_source") {
        auto* typed = static_cast<ICveSource*>(instance_ptr);
        cve_sources_[name] = std::shared_ptr<ICveSource>(
            typed, PluginDeleter<ICveSource>{raw.destroy});
        std::cout << std::format("[PluginLoader] Loaded CVE source: {} v{}\n", name, ver);
    } else if (type == "dependency_analyzer") {
        auto* typed = static_cast<IDependencyAnalyzer*>(instance_ptr);
        analyzers_[typed->ecosystem()] = std::shared_ptr<IDependencyAnalyzer>(
            typed, PluginDeleter<IDependencyAnalyzer>{raw.destroy});
        std::cout << std::format("[PluginLoader] Loaded analyzer: {} v{}\n", name, ver);
    } else {
        raw.destroy(instance_ptr);
        dl_handles_.pop_back();
        dlclose(raw.dl_handle);
        throw std::runtime_error(
            std::format("Unknown plugin_type '{}' in plugin '{}'", type, name));
    }
}

std::vector<std::shared_ptr<ICveSource>> PluginLoader::cveSources() const {
    std::vector<std::shared_ptr<ICveSource>> result;
    for (const auto& [_, src] : cve_sources_) result.push_back(src);
    return result;
}

const std::unordered_map<std::string, std::shared_ptr<ICveSource>>&
PluginLoader::cveSourcesMap() const {
    return cve_sources_;
}

std::vector<std::shared_ptr<IDependencyAnalyzer>> PluginLoader::dependencyAnalyzers() const {
    std::vector<std::shared_ptr<IDependencyAnalyzer>> result;
    for (const auto& [_, a] : analyzers_) result.push_back(a);
    return result;
}

std::shared_ptr<ICveSource> PluginLoader::cveSourceById(const std::string& id) const {
    auto it = cve_sources_.find(id);
    return it != cve_sources_.end() ? it->second : nullptr;
}

std::shared_ptr<IDependencyAnalyzer>
PluginLoader::analyzerByEcosystem(const std::string& ecosystem) const {
    auto it = analyzers_.find(ecosystem);
    return it != analyzers_.end() ? it->second : nullptr;
}

void PluginLoader::configureCveSources(const std::map<std::string, std::string>& config) {
    std::unordered_map<std::string, std::string> cfg(config.begin(), config.end());
    for (auto& [_, src] : cve_sources_) src->configure(cfg);
}

void PluginLoader::configureAnalyzers(const std::map<std::string, std::string>& config) {
    std::unordered_map<std::string, std::string> cfg(config.begin(), config.end());
    for (auto& [_, a] : analyzers_) a->configure(cfg);
}

} // namespace blueduck
