#pragma once
#include <ICveSource.h>
#include <IDependencyAnalyzer.h>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace blueduck {

class PluginLoader {
public:
    static constexpr uint32_t kSupportedApiVersion = 1;

    void loadDirectory(const std::filesystem::path& dir, const std::string& type_hint);
    void loadFile(const std::filesystem::path& path);

    std::vector<std::shared_ptr<ICveSource>>          cveSources() const;
    std::vector<std::shared_ptr<IDependencyAnalyzer>> dependencyAnalyzers() const;

    const std::unordered_map<std::string, std::shared_ptr<ICveSource>>& cveSourcesMap() const;

    std::shared_ptr<ICveSource>          cveSourceById(const std::string& id) const;
    std::shared_ptr<IDependencyAnalyzer> analyzerByEcosystem(const std::string& ecosystem) const;

    void configureCveSources(const std::map<std::string, std::string>& config);
    void configureAnalyzers(const std::map<std::string, std::string>& config);

    ~PluginLoader();

private:
    struct RawPlugin {
        void*             dl_handle;
        const PluginMeta* meta;
        void*             (*create)();
        void              (*destroy)(void*);
    };

    RawPlugin loadRaw(const std::filesystem::path& path);
    void      registerPlugin(RawPlugin raw);

    std::unordered_map<std::string, std::shared_ptr<ICveSource>>          cve_sources_;
    std::unordered_map<std::string, std::shared_ptr<IDependencyAnalyzer>> analyzers_;
    std::vector<void*>                                                      dl_handles_;
};

} // namespace blueduck
