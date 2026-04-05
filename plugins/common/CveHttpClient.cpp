#include "CveHttpClient.h"
#include <stdexcept>

namespace blueduck::plugins {

namespace {
    struct CurlGlobalGuard {
        CurlGlobalGuard()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
        ~CurlGlobalGuard() { curl_global_cleanup(); }
    };
    static CurlGlobalGuard g_curl_guard;
}

CveHttpClient::CveHttpClient() {
    curl_ = curl_easy_init();
    if (!curl_) throw std::runtime_error("curl_easy_init failed");
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
}

CveHttpClient::~CveHttpClient() {
    if (headers_) curl_slist_free_all(headers_);
    if (curl_)    curl_easy_cleanup(curl_);
}

void CveHttpClient::setHeader(const std::string& header) {
    headers_ = curl_slist_append(headers_, header.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
}

CveHttpClient::Response CveHttpClient::get(const std::string& url) const {
    Response resp;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &resp.body);

    CURLcode rc = curl_easy_perform(curl_);
    if (rc != CURLE_OK) {
        resp.error = curl_easy_strerror(rc);
        return resp;
    }
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp.status_code);
    return resp;
}

void CveHttpClient::paginate(
        const std::string& first_url,
        std::function<std::string(const std::string&)> next_url_extractor,
        std::function<bool(const std::string&)> on_page) const
{
    std::string url = first_url;
    while (!url.empty()) {
        auto resp = get(url);
        if (!resp.ok()) break;
        if (!on_page(resp.body)) break;
        url = next_url_extractor(resp.body);
    }
}

size_t CveHttpClient::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

} // namespace blueduck::plugins
