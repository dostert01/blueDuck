#pragma once
#include "Config.h"
#include "PluginLoader.h"
#include "AnalysisService.h"
#include <ICveSource.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace blueduck {

class Application {
public:
    static Application& instance();

    void init(const std::string& config_path);
    void run();

    PluginLoader&    pluginLoader();
    AnalysisService& analysisService();
    const Config&    config() const;

    /// Returns a map of CVE source name → plugin pointer (for API controllers).
    const std::unordered_map<std::string, std::shared_ptr<ICveSource>>& cveSources() const;

    /// Trigger a sync on a named CVE source (fire-and-forget, logs result).
    void syncCveSource(const std::string& name);

    /// Run dependency analysis for a project version (fire-and-forget).
    void analyzeVersion(int project_version_id);

private:
    Application() = default;

    void setupDrogon();
    void registerAngularFallback();

    Config                           config_;
    std::unique_ptr<PluginLoader>    plugin_loader_;
    std::unique_ptr<AnalysisService> analysis_service_;
    bool                             initialized_ = false;
};

} // namespace blueduck
