#pragma once
#include <string>
#include <functional>
#include <curl/curl.h>

namespace blueduck::plugins {

/// Simple synchronous HTTP GET/POST client backed by libcurl.
/// Intended for use inside CVE source plugins (no Drogon dependency).
class CveHttpClient {
public:
    struct Response {
        long        status_code{0};
        std::string body;
        std::string error;
        bool ok() const { return status_code >= 200 && status_code < 300; }
    };

    CveHttpClient();
    ~CveHttpClient();

    /// Set a request header, e.g. "apiKey: <token>"
    void setHeader(const std::string& header);

    /// Perform a synchronous GET request.
    Response get(const std::string& url) const;

    /// Perform a GET with pagination: calls \p on_page for each page body
    /// until the callback returns false or there are no more pages.
    /// \p next_url_extractor extracts the URL of the next page from the
    /// current response body; return empty string to stop.
    void paginate(const std::string& first_url,
                  std::function<std::string(const std::string& body)> next_url_extractor,
                  std::function<bool(const std::string& body)> on_page) const;

private:
    CURL*             curl_{nullptr};
    curl_slist*       headers_{nullptr};

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
};

} // namespace blueduck::plugins
