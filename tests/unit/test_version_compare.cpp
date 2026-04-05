#include <gtest/gtest.h>
#include <report/ReportEngine.h>

// Access private helpers through a thin test-only subclass
class ReportEngineTestable : public blueduck::ReportEngine {
public:
    explicit ReportEngineTestable() : ReportEngine(nullptr) {}
    using ReportEngine::parseVersion;
    using ReportEngine::versionInRange;
};

class VersionCompareTest : public ::testing::Test {
protected:
    ReportEngineTestable engine;
};

TEST_F(VersionCompareTest, ParseSimple) {
    auto v = engine.parseVersion("1.2.3");
    ASSERT_EQ(v.parts.size(), 3u);
    EXPECT_EQ(v.parts[0], 1u);
    EXPECT_EQ(v.parts[1], 2u);
    EXPECT_EQ(v.parts[2], 3u);
}

TEST_F(VersionCompareTest, ParseWithLeadingV) {
    auto v = engine.parseVersion("v1.0.0");
    ASSERT_EQ(v.parts.size(), 3u);
    EXPECT_EQ(v.parts[0], 1u);
}

TEST_F(VersionCompareTest, ParsePreRelease) {
    auto v = engine.parseVersion("2.0.0-alpha");
    EXPECT_EQ(v.parts[0], 2u);
    EXPECT_EQ(v.pre, "alpha");
}

TEST_F(VersionCompareTest, PreReleaseIsLessThanRelease) {
    auto pre     = engine.parseVersion("1.0.0-alpha");
    auto release = engine.parseVersion("1.0.0");
    EXPECT_LT(pre.compare(release), 0);
    EXPECT_GT(release.compare(pre), 0);
}

TEST_F(VersionCompareTest, PaddingShortVersion) {
    auto v100 = engine.parseVersion("1.0.0");
    auto v1   = engine.parseVersion("1");
    EXPECT_EQ(v100.compare(v1), 0);
}

TEST_F(VersionCompareTest, InRangeEndExcluding) {
    // [1.0.0, 2.0.0)
    EXPECT_TRUE(engine.versionInRange("1.5.0", "", "1.0.0", "", "", "2.0.0"));
    EXPECT_TRUE(engine.versionInRange("1.0.0", "", "1.0.0", "", "", "2.0.0"));
    EXPECT_FALSE(engine.versionInRange("2.0.0", "", "1.0.0", "", "", "2.0.0"));
    EXPECT_FALSE(engine.versionInRange("0.9.9", "", "1.0.0", "", "", "2.0.0"));
}

TEST_F(VersionCompareTest, InRangeEndIncluding) {
    // [1.0.0, 1.9.9]
    EXPECT_TRUE(engine.versionInRange("1.9.9", "", "1.0.0", "", "1.9.9", ""));
    EXPECT_FALSE(engine.versionInRange("2.0.0", "", "1.0.0", "", "1.9.9", ""));
}

TEST_F(VersionCompareTest, ExactMatch) {
    EXPECT_TRUE(engine.versionInRange("3.1.4", "3.1.4", "", "", "", ""));
    EXPECT_FALSE(engine.versionInRange("3.1.5", "3.1.4", "", "", "", ""));
}

TEST_F(VersionCompareTest, EmptyVersionNotInRange) {
    EXPECT_FALSE(engine.versionInRange("", "", "1.0.0", "", "", "2.0.0"));
}

TEST_F(VersionCompareTest, StartExcluding) {
    // (1.0.0, 2.0.0)
    EXPECT_FALSE(engine.versionInRange("1.0.0", "", "", "1.0.0", "", "2.0.0"));
    EXPECT_TRUE(engine.versionInRange("1.0.1", "", "", "1.0.0", "", "2.0.0"));
}
