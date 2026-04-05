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
};

} // namespace blueduck::plugins::cveorg
