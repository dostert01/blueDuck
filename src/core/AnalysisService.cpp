#include "AnalysisService.h"
#include <db/Database.h>
#include <format>
#include <iostream>

namespace blueduck {

AnalysisService::AnalysisService(PluginLoader& loader, const std::string& repos_dir)
    : loader_(loader)
    , git_(repos_dir)
{}

void AnalysisService::analyzeVersion(int project_version_id) {
    // Look up git info from DB, then call the full form
    auto db = Database::blockingClient();

    int    project_id = 0;
    std::string git_ref;

    try {
        auto r = db->execSqlSync(
            "SELECT pv.project_id, pv.ref_name "
            "FROM project_versions pv WHERE pv.id=$1",
            project_version_id);
        if (!r.empty()) {
            project_id = r[0][0].as<int>();
            git_ref    = r[0][1].as<std::string>();
        }
    } catch (const drogon::orm::DrogonDbException& e) {
        std::cerr << "[AnalysisService] DB error: " << e.base().what() << "\n";
    }

    if (project_id == 0 || git_ref.empty()) {
        std::cerr << "[AnalysisService] Version " << project_version_id << " not found\n";
        return;
    }

    analyzeVersion(project_id, project_version_id, git_ref);
}

bool AnalysisService::analyzeVersion(int project_id,
                                      int project_version_id,
                                      const std::string& git_ref)
{
    std::cout << std::format("[Analysis] project={} version={} ref={}\n",
                              project_id, project_version_id, git_ref);

    std::string commit_hash = git_.checkout(project_id, git_ref);
    if (commit_hash.empty()) {
        std::cerr << std::format("[Analysis] Checkout failed for ref '{}'\n", git_ref);
        return false;
    }

    std::string repo_root = git_.repoPath(project_id).string();
    std::vector<Dependency> all_deps;

    for (auto& analyzer : loader_.dependencyAnalyzers()) {
        if (!analyzer->canAnalyze(repo_root)) continue;

        std::cout << std::format("[Analysis] Running '{}' analyzer\n",
                                  analyzer->ecosystem());

        auto result = analyzer->analyze(repo_root);

        if (!result.success) {
            std::cerr << std::format("[Analysis] '{}' analyzer error: {}\n",
                                      analyzer->ecosystem(), result.error_message);
            continue;
        }

        std::cout << std::format("[Analysis] '{}' found {} dependencies\n",
                                  analyzer->ecosystem(), result.dependencies.size());

        all_deps.insert(all_deps.end(),
                         result.dependencies.begin(),
                         result.dependencies.end());
    }

    storeDependencies(project_version_id, commit_hash, all_deps);

    std::cout << std::format("[Analysis] Complete: {} total dependencies\n",
                              all_deps.size());
    return true;
}

void AnalysisService::storeDependencies(int project_version_id,
                                         const std::string& commit_hash,
                                         const std::vector<Dependency>& deps)
{
    auto db = Database::blockingClient();
    auto tx = db->newTransaction();

    try {
        tx->execSqlSync(
            "DELETE FROM dependencies WHERE project_version_id=$1",
            project_version_id);

        for (const auto& dep : deps) {
            tx->execSqlSync(
                "INSERT INTO dependencies"
                "(project_version_id, ecosystem, package_name, package_version) "
                "VALUES($1,$2,$3,$4) "
                "ON CONFLICT(project_version_id, ecosystem, package_name) "
                "DO UPDATE SET package_version=EXCLUDED.package_version",
                project_version_id,
                dep.ecosystem, dep.package_name, dep.package_version);
        }

        tx->execSqlSync(
            "UPDATE project_versions SET analyzed_at=NOW(), commit_hash=$1 WHERE id=$2",
            commit_hash, project_version_id);
    } catch (const drogon::orm::DrogonDbException& e) {
        throw std::runtime_error(
            std::string("[AnalysisService] storeDependencies failed: ") + e.base().what());
    }
}

} // namespace blueduck
