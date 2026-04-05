#pragma once
#include <interfaces/ICveSource.h>
#include <string>
#include <memory>

namespace blueduck::plugins {
class CveHttpClient;
class CveDbWriter;
class NvdParser;
}

namespace blueduck::plugins::nvd {

class NvdSource : public ICveSource {
public:
    NvdSource();
    ~NvdSource() override;

    void configure(const std::unordered_map<std::string, std::string>& cfg) override;
    SyncResult sync() override;
    std::string name() const override { return "nvd"; }

private:
    std::string api_key_;
    std::string connstr_;

    static constexpr const char* BASE_URL =
        "https://services.nvd.nist.gov/rest/json/cves/2.0";
    static constexpr int PAGE_SIZE = 2000;
};

} // namespace blueduck::plugins::nvd
