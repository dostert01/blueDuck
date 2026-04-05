#include "CredentialProvider.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <format>

namespace blueduck {

namespace {
    constexpr int kKeyLen = 32;
    constexpr int kIvLen  = 12;
    constexpr int kTagLen = 16;
}

std::vector<uint8_t> CredentialProvider::key_;

void CredentialProvider::loadKeyFromEnv() {
    const char* hex = std::getenv("BLUEDUCK_SECRET_KEY");
    if (!hex)
        throw std::runtime_error("BLUEDUCK_SECRET_KEY environment variable not set");

    std::string hex_str(hex);
    if (hex_str.size() != kKeyLen * 2)
        throw std::runtime_error(
            std::format("BLUEDUCK_SECRET_KEY must be {} hex chars (got {})",
                         kKeyLen * 2, hex_str.size()));

    key_.resize(kKeyLen);
    for (int i = 0; i < kKeyLen; ++i)
        key_[i] = static_cast<uint8_t>(std::stoi(hex_str.substr(i * 2, 2), nullptr, 16));
}

std::vector<uint8_t> CredentialProvider::encrypt(const std::string& plaintext) {
    if (key_.empty())
        throw std::runtime_error("CredentialProvider key not loaded");

    std::vector<uint8_t> iv(kIvLen);
    if (RAND_bytes(iv.data(), kIvLen) != 1)
        throw std::runtime_error("Failed to generate IV");

    std::vector<uint8_t> ciphertext(plaintext.size());
    std::vector<uint8_t> tag(kTagLen);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptInit_ex failed");
    }

    int len = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
            reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }

    int final_len = 0;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &final_len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagLen, tag.data());
    EVP_CIPHER_CTX_free(ctx);

    std::vector<uint8_t> result;
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + len + final_len);
    result.insert(result.end(), tag.begin(), tag.end());
    return result;
}

std::string CredentialProvider::decrypt(const std::vector<uint8_t>& blob) {
    if (key_.empty())
        throw std::runtime_error("CredentialProvider key not loaded");

    if (blob.size() < static_cast<size_t>(kIvLen + kTagLen))
        throw std::runtime_error("Encrypted blob too short");

    const uint8_t* iv         = blob.data();
    const uint8_t* ciphertext = blob.data() + kIvLen;
    const size_t   ct_len     = blob.size() - kIvLen - kTagLen;
    const uint8_t* tag        = blob.data() + kIvLen + ct_len;

    std::string plaintext(ct_len, '\0');

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagLen,
                         const_cast<uint8_t*>(tag));

    int len = 0;
    EVP_DecryptUpdate(ctx,
        reinterpret_cast<uint8_t*>(plaintext.data()), &len, ciphertext, ct_len);

    int final_len = 0;
    int ret = EVP_DecryptFinal_ex(ctx,
        reinterpret_cast<uint8_t*>(plaintext.data()) + len, &final_len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret != 1)
        throw std::runtime_error("Decryption failed — wrong key or corrupted data");

    plaintext.resize(len + final_len);
    return plaintext;
}

int CredentialProvider::credentialCallback(git_credential** out,
                                            const char*,
                                            const char* username_from_url,
                                            unsigned int allowed_types,
                                            void* payload)
{
    auto* creds   = static_cast<ResolvedCredentials*>(payload);
    const char* u = username_from_url ? username_from_url : "git";

    if (creds->method == ResolvedCredentials::Method::SshKey &&
        (allowed_types & GIT_CREDENTIAL_SSH_KEY))
    {
        // Prefer in-memory keys over file paths
        if (!creds->ssh_private_key.empty()) {
            return git_credential_ssh_key_memory_new(out, u,
                creds->ssh_public_key.c_str(),
                creds->ssh_private_key.c_str(),
                creds->ssh_passphrase.empty() ? nullptr : creds->ssh_passphrase.c_str());
        }
        return git_credential_ssh_key_new(out, u,
            creds->ssh_public_key_path.c_str(),
            creds->ssh_private_key_path.c_str(),
            creds->ssh_passphrase.empty() ? nullptr : creds->ssh_passphrase.c_str());
    }

    if (creds->method == ResolvedCredentials::Method::Pat &&
        (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT))
    {
        return git_credential_userpass_plaintext_new(out, "token", creds->pat.c_str());
    }

    if (allowed_types & GIT_CREDENTIAL_DEFAULT)
        return git_credential_default_new(out);

    return GIT_EAUTH;
}

} // namespace blueduck
