#pragma once
#include "PluginLoader.h"
#include <git/GitRepository.h>
#include <string>
#include <vector>

namespace blueduck {

class AnalysisService {
public:
    AnalysisService(PluginLoader& loader, const std::string& repos_dir);

    /// Fire-and-forget: looks up version in DB, then analyzes.
    void analyzeVersion(int project_version_id);

    /// Full form used internally.
    bool analyzeVersion(int project_id,
                         int project_version_id,
                         const std::string& git_ref);

private:
    void storeDependencies(int project_version_id,
                            const std::string& commit_hash,
                            const std::vector<Dependency>& deps);

    PluginLoader& loader_;
    GitRepository git_;
};

} // namespace blueduck
