#include "ProjectController.h"
#include <api/ApiHelpers.h>
#include <db/Database.h>
#include <db/FieldHelpers.h>
#include <git/GitRepository.h>
#include <git/CredentialProvider.h>
#include <core/Application.h>
#include <drogon/drogon.h>

namespace blueduck {

void ProjectController::create(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb)
{
    auto body = req->getJsonObject();
    if (!body || !(*body).isMember("name") || !(*body).isMember("git_url"))
        return cb(ApiHelpers::badRequest("name and git_url required"));

    std::string name    = (*body)["name"].asString();
    std::string git_url = (*body)["git_url"].asString();
    std::string desc    = (*body).get("description", "").asString();

    auto db = Database::client();
    *db << "INSERT INTO projects(name, git_url, description) "
           "VALUES($1,$2,$3) RETURNING id, name, git_url, description, created_at, last_synced_at"
        << name << git_url << desc
        >> [cb](const drogon::orm::Result& r) {
               cb(ApiHelpers::created(ApiHelpers::projectRowToJson(r[0])));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ProjectController::list(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb)
{
    auto db = Database::client();
    *db << "SELECT id, name, git_url, description, created_at, last_synced_at "
           "FROM projects ORDER BY name"
        >> [cb](const drogon::orm::Result& r) {
               Json::Value arr(Json::arrayValue);
               for (const auto& row : r)
                   arr.append(ApiHelpers::projectRowToJson(row));
               cb(ApiHelpers::ok(arr));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ProjectController::get(const drogon::HttpRequestPtr&,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             int id)
{
    auto db = Database::client();
    *db << "SELECT id, name, git_url, description, created_at, last_synced_at "
           "FROM projects WHERE id=$1" << id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("project"));
               cb(ApiHelpers::ok(ApiHelpers::projectRowToJson(r[0])));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ProjectController::remove(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                int id)
{
    auto db = Database::client();
    *db << "DELETE FROM projects WHERE id=$1 RETURNING id" << id
        >> [cb](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("project"));
               cb(ApiHelpers::noContent());
           }
        >> [cb](const drogon::orm::DrogonDbException& e) {
               cb(ApiHelpers::dbError(e));
           };
}

void ProjectController::setCredentials(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                        int id)
{
    auto body = req->getJsonObject();
    if (!body || !(*body).isMember("auth_method"))
        return cb(ApiHelpers::badRequest("auth_method required"));

    std::string method = (*body)["auth_method"].asString();
    auto db = Database::client();

    if (method == "pat") {
        std::string pat = (*body).get("pat", "").asString();
        if (pat.empty()) return cb(ApiHelpers::badRequest("pat required"));
        auto enc = CredentialProvider::encrypt(pat);
        std::string enc_str(enc.begin(), enc.end());

        *db << "INSERT INTO project_credentials(project_id, auth_method, pat_enc) "
               "VALUES($1,'pat',$2) ON CONFLICT(project_id) DO UPDATE "
               "SET auth_method='pat', pat_enc=$2, "
               "ssh_private_key_path=NULL, ssh_public_key_path=NULL, ssh_passphrase_enc=NULL"
            << id << enc_str
            >> [cb](const drogon::orm::Result&) { cb(ApiHelpers::noContent()); }
            >> [cb](const drogon::orm::DrogonDbException& e) { cb(ApiHelpers::dbError(e)); };

    } else if (method == "ssh_key") {
        std::string priv = (*body).get("ssh_private_key", "").asString();
        std::string pub  = (*body).get("ssh_public_key",  "").asString();
        if (priv.empty() || pub.empty())
            return cb(ApiHelpers::badRequest(
                "ssh_private_key and ssh_public_key required"));

        auto priv_enc = CredentialProvider::encrypt(priv);
        auto pub_enc  = CredentialProvider::encrypt(pub);
        std::string priv_enc_str(priv_enc.begin(), priv_enc.end());
        std::string pub_enc_str(pub_enc.begin(), pub_enc.end());

        *db << "INSERT INTO project_credentials"
               "(project_id, auth_method, ssh_private_key_enc, ssh_public_key_enc) "
               "VALUES($1,'ssh_key',$2,$3) ON CONFLICT(project_id) DO UPDATE "
               "SET auth_method='ssh_key', ssh_private_key_enc=$2, ssh_public_key_enc=$3,"
               " ssh_private_key_path=NULL, ssh_public_key_path=NULL,"
               " pat_enc=NULL, ssh_passphrase_enc=NULL"
            << id << priv_enc_str << pub_enc_str
            >> [cb](const drogon::orm::Result&) { cb(ApiHelpers::noContent()); }
            >> [cb](const drogon::orm::DrogonDbException& e) { cb(ApiHelpers::dbError(e)); };

    } else if (method == "none") {
        *db << "INSERT INTO project_credentials(project_id, auth_method) "
               "VALUES($1,'none') ON CONFLICT(project_id) DO UPDATE SET auth_method='none'"
            << id
            >> [cb](const drogon::orm::Result&) { cb(ApiHelpers::noContent()); }
            >> [cb](const drogon::orm::DrogonDbException& e) { cb(ApiHelpers::dbError(e)); };
    } else {
        cb(ApiHelpers::badRequest("auth_method must be: none, pat, ssh_key"));
    }
}

void ProjectController::sync(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              int id)
{
    auto db = Database::client();
    *db << "SELECT p.git_url, pc.auth_method, "
           "pc.ssh_private_key_path, pc.ssh_public_key_path, pc.pat_enc, "
           "pc.ssh_private_key_enc, pc.ssh_public_key_enc "
           "FROM projects p "
           "LEFT JOIN project_credentials pc ON pc.project_id = p.id "
           "WHERE p.id=$1" << id
        >> [cb, id](const drogon::orm::Result& r) {
               if (r.empty()) return cb(ApiHelpers::notFound("project"));

               ResolvedCredentials creds;
               std::string method = fieldOr(r[0][1], std::string("none"));

               if (method == "ssh_key") {
                   creds.method = ResolvedCredentials::Method::SshKey;
                   // Prefer encrypted key content over file paths
                   if (!r[0][5].isNull()) {
                       std::string enc_str = r[0][5].as<std::string>();
                       std::vector<uint8_t> enc(enc_str.begin(), enc_str.end());
                       creds.ssh_private_key = CredentialProvider::decrypt(enc);
                   } else if (!r[0][2].isNull()) {
                       creds.ssh_private_key_path = r[0][2].as<std::string>();
                   }
                   if (!r[0][6].isNull()) {
                       std::string enc_str = r[0][6].as<std::string>();
                       std::vector<uint8_t> enc(enc_str.begin(), enc_str.end());
                       creds.ssh_public_key = CredentialProvider::decrypt(enc);
                   } else if (!r[0][3].isNull()) {
                       creds.ssh_public_key_path = r[0][3].as<std::string>();
                   }
               } else if (method == "pat" && !r[0][4].isNull()) {
                   creds.method = ResolvedCredentials::Method::Pat;
                   std::string enc_str = r[0][4].as<std::string>();
                   std::vector<uint8_t> enc(enc_str.begin(), enc_str.end());
                   creds.pat = CredentialProvider::decrypt(enc);
               }

               const std::string& repos_dir =
                   Application::instance().config().repos_dir;

               drogon::async_run([id, git_url = r[0][0].as<std::string>(),
                                   creds, repos_dir]() -> drogon::Task<> {
                   GitRepository repo(repos_dir);
                   auto result = repo.cloneOrFetch(id, git_url, creds);
                   if (result.success) {
                       // Discover branches and tags, upsert into project_versions
                       auto refs = repo.listRefs(id);
                       auto db = Database::client();
                       for (const auto& ref : refs) {
                           *db << "INSERT INTO project_versions"
                                  "(project_id, ref_name, ref_type, commit_hash) "
                                  "VALUES($1,$2,$3,$4) "
                                  "ON CONFLICT(project_id, ref_name) "
                                  "DO UPDATE SET commit_hash=EXCLUDED.commit_hash"
                               << id << ref.name << ref.type << ref.commit_hash
                               >> [](const drogon::orm::Result&) {}
                               >> [](const drogon::orm::DrogonDbException& e) {
                                      std::cerr << "[Sync] version upsert error: "
                                                << e.base().what() << "\n";
                                  };
                       }
                       *db << "UPDATE projects SET last_synced_at=NOW() WHERE id=$1"
                           << id
                           >> [](const drogon::orm::Result&) {}
                           >> [](const drogon::orm::DrogonDbException&) {};
                       std::cout << std::format("[Sync] project {} — discovered {} refs\n",
                                                 id, refs.size());
                   }
                   co_return;
               });

               Json::Value resp; resp["status"] = "sync started";
               cb(ApiHelpers::accepted(resp));
           }
        >> [cb](const drogon::orm::DrogonDbException& e) { cb(ApiHelpers::dbError(e)); };
}

} // namespace blueduck
