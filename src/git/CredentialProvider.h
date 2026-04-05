#pragma once
#include <git2.h>
#include <string>
#include <vector>
#include <cstdint>

namespace blueduck {

struct ResolvedCredentials {
    enum class Method { None, SshKey, Pat };

    Method      method = Method::None;
    std::string ssh_private_key_path;
    std::string ssh_public_key_path;
    std::string ssh_private_key;       // PEM content (stored encrypted in DB)
    std::string ssh_public_key;        // public key content
    std::string ssh_passphrase;
    std::string pat;
};

class CredentialProvider {
public:
    static void loadKeyFromEnv();
    static std::vector<uint8_t> encrypt(const std::string& plaintext);
    static std::string decrypt(const std::vector<uint8_t>& ciphertext);

    static int credentialCallback(git_credential** out,
                                   const char*      url,
                                   const char*      username_from_url,
                                   unsigned int     allowed_types,
                                   void*            payload);
private:
    static std::vector<uint8_t> key_;
};

} // namespace blueduck
