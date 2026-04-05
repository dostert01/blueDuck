#include "Application.h"
#include <db/Database.h>
#include <db/Migrator.h>
#include <git/CredentialProvider.h>
#include <drogon/drogon.h>
#include <stdexcept>
#include <format>
#include <iostream>
#include <csignal>
#include <thread>

namespace blueduck {

Application& Application::instance() {
    static Application app;
    return app;
}

const Config&    Application::config()          const { return config_; }
PluginLoader&    Application::pluginLoader()          { return *plugin_loader_; }
AnalysisService& Application::analysisService()       { return *analysis_service_; }

const std::unordered_map<std::string, std::shared_ptr<ICveSource>>&
Application::cveSources() const {
    return plugin_loader_->cveSourcesMap();
}

void Application::syncCveSource(const std::string& name) {
    auto src = plugin_loader_->cveSourceById(name);
    if (!src) {
        std::cerr << "[Application] CVE source '" << name << "' not found\n";
        return;
    }
    std::cout << "[Application] Starting CVE sync: " << name << "\n";
    auto result = src->sync();
    if (result.success)
        std::cout << std::format("[Application] CVE sync '{}' done: {} records\n",
                                  name, result.records_added);
    else
        std::cerr << "[Application] CVE sync '" << name << "' failed: "
                  << result.error_message << "\n";
}

void Application::analyzeVersion(int project_version_id) {
    analysis_service_->analyzeVersion(project_version_id);
}

void Application::init(const std::string& config_path) {
    if (initialized_)
        throw std::runtime_error("Application already initialized");

    std::cout << "[blueDuck] Loading config from: " << config_path << "\n";
    config_ = loadConfig(config_path);

    std::cout << "[blueDuck] Loading secret key\n";
    CredentialProvider::loadKeyFromEnv();

    std::cout << std::format("[blueDuck] Connecting to database {}:{}/{}\n",
                              config_.db.host, config_.db.port, config_.db.name);
    Database::configure(config_.db);

    std::cout << "[blueDuck] Running migrations\n";
    Migrator migrator(Database::connectionString(), config_.migrations_dir);
    migrator.migrate();

    std::cout << "[blueDuck] Loading plugins from: " << config_.plugin_dir << "\n";
    plugin_loader_ = std::make_unique<PluginLoader>();

    plugin_loader_->loadDirectory(
        std::filesystem::path(config_.plugin_dir) / "cve_source",
        "cve_source");
    plugin_loader_->loadDirectory(
        std::filesystem::path(config_.plugin_dir) / "dependency_analyzer",
        "dependency_analyzer");

    plugin_loader_->configureCveSources(config_.plugin_config);
    plugin_loader_->configureAnalyzers(config_.plugin_config);

    analysis_service_ = std::make_unique<AnalysisService>(
        *plugin_loader_, config_.repos_dir);

    setupDrogon();

    initialized_ = true;
    std::cout << "[blueDuck] Initialized — listening on port "
              << config_.server.port << "\n";
}

void Application::setupDrogon() {
    int threads = config_.server.threads > 0
                  ? config_.server.threads
                  : static_cast<int>(std::thread::hardware_concurrency());

    drogon::app()
        .setLogLevel(trantor::Logger::kInfo)
        .addListener("0.0.0.0", config_.server.port)
        .setThreadNum(threads);

    if (!config_.server.document_root.empty()) {
        drogon::app()
            .setDocumentRoot(config_.server.document_root)
            .setStaticFilesCacheTime(3600);

        registerAngularFallback();
        std::cout << "[blueDuck] Serving frontend from: "
                  << config_.server.document_root << "\n";
    }
}

void Application::registerAngularFallback() {
    drogon::app().registerHandler(
        "/{path}",
        [root = config_.server.document_root](
            const drogon::HttpRequestPtr&,
            std::function<void(const drogon::HttpResponsePtr&)>&& cb,
            const std::string& path)
        {
            auto file = std::filesystem::path(root) / path;
            if (std::filesystem::exists(file) && std::filesystem::is_regular_file(file)) {
                cb(drogon::HttpResponse::newFileResponse(file.string()));
                return;
            }
            cb(drogon::HttpResponse::newFileResponse(root + "/index.html"));
        },
        {drogon::Get});
}

void Application::run() {
    if (!initialized_)
        throw std::runtime_error("Call init() before run()");

    std::signal(SIGTERM, [](int) {
        std::cout << "\n[blueDuck] Shutting down\n";
        drogon::app().quit();
    });
    std::signal(SIGINT, [](int) {
        std::cout << "\n[blueDuck] Shutting down\n";
        drogon::app().quit();
    });

    std::cout << "[blueDuck] Running\n";
    drogon::app().run();
    std::cout << "[blueDuck] Stopped\n";
}

} // namespace blueduck
