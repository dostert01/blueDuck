#include <gtest/gtest.h>
#include <plugins/cve_sources/cveorg/CveOrgParser.h>

using blueduck::plugins::cveorg::CveOrgParser;

// CVE 5.0 record whose `affected` entry carries an npm collectionURL.
// `packageName` is the real registry package name; `product` is a
// human-readable name that can legitimately differ from it.
static const std::string CVEORG_NPM_SAMPLE = R"({
  "cveMetadata": {
    "cveId": "CVE-2024-11111",
    "datePublished": "2024-02-01T00:00:00Z",
    "dateUpdated": "2024-02-02T00:00:00Z"
  },
  "containers": {
    "cna": {
      "descriptions": [
        {"lang": "en", "value": "A prototype pollution issue."}
      ],
      "metrics": [
        {"cvssV3_1": {"baseScore": 7.5, "baseSeverity": "HIGH"}}
      ],
      "affected": [
        {
          "vendor": "Example Project",
          "product": "Example JS Utility Library",
          "packageName": "example-js-lib",
          "collectionURL": "https://registry.npmjs.org/",
          "versions": [
            {"version": "0", "lessThan": "2.1.0", "status": "affected", "versionType": "semver"}
          ]
        }
      ]
    }
  }
})";

// Entry with no packageName at all — package name must fall back to `product`.
static const std::string CVEORG_NO_PACKAGENAME_SAMPLE = R"({
  "cveMetadata": {
    "cveId": "CVE-2024-22222",
    "datePublished": "2024-03-01T00:00:00Z",
    "dateUpdated": "2024-03-02T00:00:00Z"
  },
  "containers": {
    "cna": {
      "descriptions": [
        {"lang": "en", "value": "An issue with no packageName field."}
      ],
      "affected": [
        {
          "vendor": "Example Vendor",
          "product": "exampleproduct",
          "collectionURL": "https://pypi.org/project/exampleproduct/",
          "versions": [
            {"version": "1.0.0", "status": "affected"}
          ]
        }
      ]
    }
  }
})";

// Entry whose collectionURL isn't one of the registries blueDuck recognizes.
static const std::string CVEORG_UNKNOWN_REGISTRY_SAMPLE = R"({
  "cveMetadata": {
    "cveId": "CVE-2024-33333",
    "datePublished": "2024-04-01T00:00:00Z",
    "dateUpdated": "2024-04-02T00:00:00Z"
  },
  "containers": {
    "cna": {
      "descriptions": [
        {"lang": "en", "value": "An issue in a package from an unknown registry."}
      ],
      "affected": [
        {
          "vendor": "Some Vendor",
          "product": "some-product",
          "packageName": "some-product",
          "collectionURL": "https://example-internal-registry.local/",
          "versions": [
            {"version": "1.0.0", "status": "affected"}
          ]
        }
      ]
    }
  }
})";

TEST(CveOrgParserTest, ParsesCveMetadata) {
    auto cve = CveOrgParser::parse(CVEORG_NPM_SAMPLE);
    ASSERT_TRUE(cve.has_value());
    EXPECT_EQ(cve->cve_id, "CVE-2024-11111");
    EXPECT_EQ(cve->description, "A prototype pollution issue.");
}

TEST(CveOrgParserTest, UsesPackageNameNotProductForPackageName) {
    auto cve = CveOrgParser::parse(CVEORG_NPM_SAMPLE);
    ASSERT_TRUE(cve.has_value());
    ASSERT_EQ(cve->affected.size(), 1u);
    // packageName ("example-js-lib"), not product ("Example JS Utility Library").
    EXPECT_EQ(cve->affected[0].package_name, "example-js-lib");
}

TEST(CveOrgParserTest, InfersNpmEcosystemFromCollectionUrl) {
    auto cve = CveOrgParser::parse(CVEORG_NPM_SAMPLE);
    ASSERT_TRUE(cve.has_value());
    ASSERT_EQ(cve->affected.size(), 1u);
    EXPECT_EQ(cve->affected[0].ecosystem, "npm");
}

TEST(CveOrgParserTest, FallsBackToProductWhenPackageNameMissing) {
    auto cve = CveOrgParser::parse(CVEORG_NO_PACKAGENAME_SAMPLE);
    ASSERT_TRUE(cve.has_value());
    ASSERT_EQ(cve->affected.size(), 1u);
    EXPECT_EQ(cve->affected[0].package_name, "exampleproduct");
    EXPECT_EQ(cve->affected[0].ecosystem, "pypi");
}

TEST(CveOrgParserTest, UnknownRegistryYieldsEmptyEcosystem) {
    auto cve = CveOrgParser::parse(CVEORG_UNKNOWN_REGISTRY_SAMPLE);
    ASSERT_TRUE(cve.has_value());
    ASSERT_EQ(cve->affected.size(), 1u);
    EXPECT_TRUE(cve->affected[0].ecosystem.empty());
    EXPECT_EQ(cve->affected[0].package_name, "some-product");
}

TEST(CveOrgParserTest, InvalidJsonReturnsNullopt) {
    auto cve = CveOrgParser::parse("not json {{{{");
    EXPECT_FALSE(cve.has_value());
}
