#include <gtest/gtest.h>
#include <plugins/cve_sources/nvd/NvdParser.h>

using blueduck::plugins::nvd::NvdParser;

static const std::string NVD_SAMPLE = R"({
  "resultsPerPage": 1,
  "startIndex": 0,
  "totalResults": 1,
  "vulnerabilities": [
    {
      "cve": {
        "id": "CVE-2023-99999",
        "published": "2023-01-15T00:00:00.000",
        "lastModified": "2023-06-01T12:00:00.000",
        "descriptions": [
          {"lang": "en", "value": "A critical buffer overflow."}
        ],
        "metrics": {
          "cvssMetricV31": [
            {
              "cvssData": {
                "baseScore": 9.8,
                "baseSeverity": "CRITICAL"
              }
            }
          ]
        },
        "configurations": [
          {
            "nodes": [
              {
                "cpeMatch": [
                  {
                    "vulnerable": true,
                    "criteria": "cpe:2.3:a:example:lib:*:*:*:*:*:*:*:*",
                    "versionStartIncluding": "1.0.0",
                    "versionEndExcluding": "2.0.0"
                  }
                ]
              }
            ]
          }
        ]
      }
    }
  ]
})";

TEST(NvdParserTest, ParsesSingleCve) {
    auto cves = NvdParser::parse(NVD_SAMPLE);
    ASSERT_EQ(cves.size(), 1u);

    const auto& cve = cves[0];
    EXPECT_EQ(cve.cve_id, "CVE-2023-99999");
    EXPECT_EQ(cve.description, "A critical buffer overflow.");
    EXPECT_EQ(cve.severity, "CRITICAL");
    ASSERT_TRUE(cve.cvss_v3_score.has_value());
    EXPECT_FLOAT_EQ(*cve.cvss_v3_score, 9.8f);
}

TEST(NvdParserTest, ParsesAffectedRange) {
    auto cves = NvdParser::parse(NVD_SAMPLE);
    ASSERT_EQ(cves.size(), 1u);
    ASSERT_EQ(cves[0].affected.size(), 1u);

    const auto& r = cves[0].affected[0];
    EXPECT_EQ(r.cpe_uri, "cpe:2.3:a:example:lib:*:*:*:*:*:*:*:*");
    ASSERT_TRUE(r.version_start_inc.has_value());
    EXPECT_EQ(*r.version_start_inc, "1.0.0");
    ASSERT_TRUE(r.version_end_exc.has_value());
    EXPECT_EQ(*r.version_end_exc, "2.0.0");
}

TEST(NvdParserTest, NextPageUrlReturnsEmptyOnLastPage) {
    std::string url = NvdParser::nextPageUrl(
        "https://services.nvd.nist.gov/rest/json/cves/2.0", 2000, NVD_SAMPLE);
    EXPECT_TRUE(url.empty());
}

TEST(NvdParserTest, NextPageUrlReturnNextStartIndex) {
    std::string body = R"({
        "resultsPerPage": 2000,
        "startIndex": 0,
        "totalResults": 3000
    })";
    std::string url = NvdParser::nextPageUrl(
        "https://services.nvd.nist.gov/rest/json/cves/2.0", 2000, body);
    EXPECT_NE(url.find("startIndex=2000"), std::string::npos);
}

TEST(NvdParserTest, InvalidJsonReturnsEmpty) {
    auto cves = NvdParser::parse("not json {{{{");
    EXPECT_TRUE(cves.empty());
}
