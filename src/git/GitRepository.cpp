#include "GitRepository.h"
#include <format>
#include <iostream>

namespace blueduck {

namespace {
    struct Git2Guard {
        Git2Guard()  { git_libgit2_init(); }
        ~Git2Guard() { git_libgit2_shutdown(); }
    };
    static Git2Guard g_git2_guard;
}

GitRepository::GitRepository(std::filesystem::path repos_base_dir)
    : repos_base_dir_(std::move(repos_base_dir))
{
    std::filesystem::create_directories(repos_base_dir_);
}

GitRepository::~GitRepository() = default;

std::string GitRepository::git2Error(const std::string& context) {
    const git_error* e = git_error_last();
    return std::format("{}: {}", context, e ? e->message : "unknown error");
}

std::filesystem::path GitRepository::repoPath(int project_id) const {
    return repos_base_dir_ / std::to_string(project_id);
}

CloneResult GitRepository::cloneOrFetch(int project_id,
                                         const std::string& git_url,
                                         const ResolvedCredentials& creds)
{
    auto path = repoPath(project_id);
    if (std::filesystem::exists(path / ".git")) {
        git_repository* repo = openRepo(project_id);
        auto result = doFetch(repo, creds);
        git_repository_free(repo);
        if (!result.success) return { false, result.error_message, "" };
        return { true, "", "" };
    }
    return doClone(project_id, git_url, creds);
}

CloneResult GitRepository::doClone(int project_id,
                                    const std::string& git_url,
                                    const ResolvedCredentials& creds)
{
    auto path = repoPath(project_id);
    auto mutable_creds = creds;

    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    opts.fetch_opts.callbacks.credentials = &CredentialProvider::credentialCallback;
    opts.fetch_opts.callbacks.payload     = &mutable_creds;
    opts.checkout_opts.checkout_strategy  = GIT_CHECKOUT_SAFE;

    git_repository* repo = nullptr;
    int rc = git_clone(&repo, git_url.c_str(), path.c_str(), &opts);
    if (rc != 0) return { false, git2Error("Clone failed"), "" };

    std::string hash = resolveCommitHash(repo, "HEAD");
    git_repository_free(repo);
    std::cout << std::format("[Git] Cloned project {} to {}\n",
                              project_id, path.string());
    return { true, "", hash };
}

FetchResult GitRepository::doFetch(git_repository* repo,
                                    const ResolvedCredentials& creds)
{
    auto mutable_creds = creds;
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, repo, "origin") != 0)
        return { false, git2Error("Remote 'origin' not found") };

    git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
    opts.callbacks.credentials = &CredentialProvider::credentialCallback;
    opts.callbacks.payload     = &mutable_creds;

    int rc = git_remote_fetch(remote, nullptr, &opts, nullptr);
    git_remote_free(remote);
    if (rc != 0) return { false, git2Error("Fetch failed") };
    return { true, "" };
}

std::string GitRepository::checkout(int project_id, const std::string& git_ref) {
    git_repository* repo = openRepo(project_id);
    if (!repo) return "";

    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo, git_ref.c_str()) != 0) {
        std::cerr << git2Error(std::format("Cannot resolve ref '{}'", git_ref)) << "\n";
        git_repository_free(repo);
        return "";
    }

    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy    = GIT_CHECKOUT_FORCE;

    if (git_checkout_tree(repo, obj, &opts) != 0) {
        std::cerr << git2Error("Checkout tree failed") << "\n";
        git_object_free(obj);
        git_repository_free(repo);
        return "";
    }

    git_repository_set_head_detached(repo, git_object_id(obj));
    std::string hash = resolveCommitHash(repo, git_ref);
    git_object_free(obj);
    git_repository_free(repo);
    return hash;
}

std::vector<GitRef> GitRepository::listRefs(int project_id) {
    std::vector<GitRef> refs;
    git_repository* repo = openRepo(project_id);
    if (!repo) return refs;

    // List branches
    git_branch_iterator* iter = nullptr;
    if (git_branch_iterator_new(&iter, repo, GIT_BRANCH_LOCAL) == 0) {
        git_reference* ref = nullptr;
        git_branch_t type;
        while (git_branch_next(&ref, &type, iter) == 0) {
            const char* name = nullptr;
            git_branch_name(&name, ref);
            if (name) {
                std::string hash = resolveCommitHash(repo, std::string("refs/heads/") + name);
                refs.push_back({name, "branch", hash});
            }
            git_reference_free(ref);
        }
        git_branch_iterator_free(iter);
    }

    // List tags
    git_strarray tag_names = {nullptr, 0};
    if (git_tag_list(&tag_names, repo) == 0) {
        for (size_t i = 0; i < tag_names.count; ++i) {
            std::string hash = resolveCommitHash(repo, std::string("refs/tags/") + tag_names.strings[i]);
            refs.push_back({tag_names.strings[i], "tag", hash});
        }
        git_strarray_dispose(&tag_names);
    }

    git_repository_free(repo);
    return refs;
}

git_repository* GitRepository::openRepo(int project_id) {
    git_repository* repo = nullptr;
    auto path = repoPath(project_id);
    if (git_repository_open(&repo, path.c_str()) != 0) {
        std::cerr << git2Error(
            std::format("Cannot open repo at {}", path.string())) << "\n";
        return nullptr;
    }
    return repo;
}

std::string GitRepository::resolveCommitHash(git_repository* repo,
                                              const std::string& git_ref)
{
    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo, git_ref.c_str()) != 0) return "";
    const git_oid* oid = git_object_id(obj);
    char hash[GIT_OID_HEXSZ + 1];
    git_oid_tostr(hash, sizeof(hash), oid);
    git_object_free(obj);
    return std::string(hash);
}

} // namespace blueduck
