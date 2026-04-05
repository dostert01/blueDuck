#pragma once
#include <plugins/common/CveDbWriter.h>
#include <string>
#include <vector>

namespace blueduck::plugins::nvd {

/// Parses an NVD API 2.0 JSON response page and returns CveData records.
class NvdParser {
public:
    /// Parse a single NVD API response page.
    /// Returns the list of parsed CVEs.
    static std::vector<CveDbWriter::CveData> parse(const std::string& json_body);

    /// Extract the startIndex for the next page from the response body.
    /// Returns empty string when all pages have been consumed.
    static std::string nextPageUrl(const std::string& base_url,
                                    int page_size,
                                    const std::string& json_body);

private:
    static std::string severityFromCvssV3(float score);
    static std::string severityFromCvssV2(float score);
};

} // namespace blueduck::plugins::nvd
