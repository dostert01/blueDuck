#pragma once
#include "PluginMeta.h"
#include <string>
#include <unordered_map>

namespace blueduck {

struct SyncResult {
    bool        success        = false;
    std::string error_message;
    int         records_added  = 0;
    int         records_failed = 0;
};

class ICveSource {
public:
    virtual ~ICveSource() = default;

    /// Return a unique machine-readable name, e.g. "nvd", "cveorg".
    virtual std::string name() const = 0;

    /// Called once after loading; passes db_connstr, api keys, etc.
    virtual void configure(const std::unordered_map<std::string, std::string>& config) = 0;

    /// Fetch/update CVE data and write directly to the database.
    virtual SyncResult sync() = 0;
};

} // namespace blueduck
