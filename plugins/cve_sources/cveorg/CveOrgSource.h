#pragma once
#include <interfaces/ICveSource.h>
#include <plugins/common/CveDbWriter.h>
#include <string>

// Forward-declared rather than pulling in <zip.h> here, since it's only
// used by the .cpp.
struct zip;
typedef struct zip zip_t;

namespace blueduck::plugins::cveorg {

/// CVE source plugin that downloads the CVE.org bulk download (JSON 5.0 format).
/// Downloads the delta feed daily; full download on first run or when forced.
class CveOrgSource : public ICveSource {
public:
    CveOrgSource();
    ~CveOrgSource() override;

    void configure(const std::unordered_map<std::string, std::string>& cfg) override;
    SyncResult sync() override;
    std::string name() const override { return "cveorg"; }

private:
    std::string connstr_;
    std::string cache_dir_;

    static constexpr const char* RELEASES_API_URL =
        "https://api.github.com/repos/CVEProject/cvelistV5/releases/latest";

    struct ReleaseAssets {
        std::string delta_url;
        std::string full_url;
    };

    ReleaseAssets discoverAssetUrls();
    SyncResult processBulkZip(const std::string& zip_path);
    bool downloadFile(const std::string& url, const std::string& dest_path);

    /// Walks every entry of an already-open zip archive. CVE JSON 5.0 files
    /// are parsed and written via `writer`; nested .zip entries (the
    /// CVEProject "all CVEs" release wraps its data in a zip-within-a-zip)
    /// are recursed into, up to a small depth limit.
    void processZipEntries(zip_t* za, CveDbWriter& writer, SyncResult& result,
                            int depth = 0);
};

} // namespace blueduck::plugins::cveorg
