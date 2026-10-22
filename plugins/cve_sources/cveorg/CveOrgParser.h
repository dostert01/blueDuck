#pragma once
#include <plugins/common/CveDbWriter.h>
#include <string>
#include <vector>

namespace blueduck::plugins::cveorg {

/// Parses CVE.org JSON 5.0 format (single CVE file) into CveDbWriter::CveData.
class CveOrgParser {
public:
    /// Parse a single CVE JSON 5.0 file content.
    static std::optional<CveDbWriter::CveData> parse(const std::string& json_body);

private:
    static std::string resolveSeverity(const std::string& json_body);

    /// Infer a blueDuck ecosystem string ("npm", "pypi", "cargo", "nuget")
    /// from a CVE 5.0 `affected[].collectionURL`. Returns "" when the
    /// registry is unknown/unsupported — CVE 5.0 has no dedicated
    /// ecosystem field, so the package registry URL is the only signal.
    static std::string ecosystemFromCollectionUrl(const std::string& url);
};

} // namespace blueduck::plugins::cveorg
