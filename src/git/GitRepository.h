#pragma once
#include "CredentialProvider.h"
#include <git2.h>
#include <filesystem>
#include <string>

namespace blueduck {

struct CloneResult {
    bool        success;
    std::string error_message;
    std::string commit_hash;
};

struct FetchResult {
    bool        success;
    std::string error_message;
};

struct GitRef {
    std::string name;      // e.g. "main", "v1.0.0"
    std::string type;      // "branch" or "tag"
    std::string commit_hash;
};

class GitRepository {
public:
    explicit GitRepository(std::filesystem::path repos_base_dir);
    ~GitRepository();

    CloneResult cloneOrFetch(int project_id,
                              const std::string& git_url,
                              const ResolvedCredentials& creds);

    std::string checkout(int project_id, const std::string& git_ref);
    std::vector<GitRef> listRefs(int project_id);
    std::filesystem::path repoPath(int project_id) const;

private:
    git_repository* openRepo(int project_id);
    CloneResult     doClone(int project_id, const std::string& git_url,
                             const ResolvedCredentials& creds);
    FetchResult     doFetch(git_repository* repo, const ResolvedCredentials& creds);

    static std::string resolveCommitHash(git_repository* repo,
                                          const std::string& git_ref);
    static std::string git2Error(const std::string& context);

    std::filesystem::path repos_base_dir_;
};

} // namespace blueduck
