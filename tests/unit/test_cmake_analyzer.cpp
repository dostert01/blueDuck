#include <gtest/gtest.h>
#include <plugins/dependency_analyzers/cmake_cpp/CmakeAnalyzer.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using blueduck::plugins::cmake_cpp::CmakeAnalyzer;

class CmakeAnalyzerTest : public ::testing::Test {
protected:
    CmakeAnalyzer analyzer;
    std::string tmp_dir;

    void SetUp() override {
        tmp_dir = fs::temp_directory_path() / "blueduck_cmake_test";
        fs::create_directories(tmp_dir);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir);
    }

    void writeFile(const std::string& rel, const std::string& content) {
        auto path = tmp_dir + "/" + rel;
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream f(path);
        f << content;
    }
};

TEST_F(CmakeAnalyzerTest, CanAnalyzeWithCMakeListsTxt) {
    writeFile("CMakeLists.txt", "cmake_minimum_required(VERSION 3.20)\n");
    EXPECT_TRUE(analyzer.canAnalyze(tmp_dir));
}

TEST_F(CmakeAnalyzerTest, CannotAnalyzeWithoutCMakeLists) {
    EXPECT_FALSE(analyzer.canAnalyze(tmp_dir));
}

TEST_F(CmakeAnalyzerTest, FindPackageParsed) {
    writeFile("CMakeLists.txt",
        "find_package(ZLIB REQUIRED)\n"
        "find_package(OpenSSL 3.0)\n");

    auto result = analyzer.analyze(tmp_dir);
    EXPECT_TRUE(result.success);

    auto has = [&](const std::string& name) {
        return std::any_of(result.dependencies.begin(), result.dependencies.end(),
            [&](const auto& d) { return d.package_name == name; });
    };
    EXPECT_TRUE(has("ZLIB"));
    EXPECT_TRUE(has("OpenSSL"));
}

TEST_F(CmakeAnalyzerTest, FindPackageVersionCaptured) {
    writeFile("CMakeLists.txt", "find_package(Boost 1.80 REQUIRED)\n");

    auto result = analyzer.analyze(tmp_dir);
    ASSERT_FALSE(result.dependencies.empty());
    auto it = std::find_if(result.dependencies.begin(), result.dependencies.end(),
        [](const auto& d) { return d.package_name == "Boost"; });
    ASSERT_NE(it, result.dependencies.end());
    EXPECT_EQ(it->package_version, "1.80");
}

TEST_F(CmakeAnalyzerTest, FetchContentDeclareDetected) {
    writeFile("CMakeLists.txt",
        "include(FetchContent)\n"
        "FetchContent_Declare(\n"
        "  googletest\n"
        "  GIT_REPOSITORY https://github.com/google/googletest.git\n"
        "  GIT_TAG v1.14.0\n"
        ")\n");

    auto result = analyzer.analyze(tmp_dir);
    EXPECT_TRUE(result.success);
    auto it = std::find_if(result.dependencies.begin(), result.dependencies.end(),
        [](const auto& d) { return d.package_name == "googletest"; });
    ASSERT_NE(it, result.dependencies.end());
    EXPECT_EQ(it->package_version, "v1.14.0");
}
