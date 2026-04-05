#include <gtest/gtest.h>
#include <plugins/dependency_analyzers/python/PythonAnalyzer.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using blueduck::plugins::python::PythonAnalyzer;

class PythonAnalyzerTest : public ::testing::Test {
protected:
    PythonAnalyzer analyzer;
    std::string tmp_dir;

    void SetUp() override {
        tmp_dir = fs::temp_directory_path() / "blueduck_python_test";
        fs::create_directories(tmp_dir);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir);
    }

    void writeFile(const std::string& name, const std::string& content) {
        std::ofstream f(tmp_dir + "/" + name);
        f << content;
    }

    bool hasDep(const std::vector<blueduck::Dependency>& deps,
                const std::string& name) {
        return std::any_of(deps.begin(), deps.end(),
            [&](const auto& d) { return d.package_name == name; });
    }
};

TEST_F(PythonAnalyzerTest, ParsesRequirementsTxt) {
    writeFile("requirements.txt",
        "requests==2.31.0\n"
        "flask>=2.0\n"
        "# a comment\n"
        "numpy\n");

    auto result = analyzer.analyze(tmp_dir);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(hasDep(result.dependencies, "requests"));
    EXPECT_TRUE(hasDep(result.dependencies, "flask"));
    EXPECT_TRUE(hasDep(result.dependencies, "numpy"));
}

TEST_F(PythonAnalyzerTest, NormalizesPackageName) {
    writeFile("requirements.txt", "Pillow==9.0.0\nSome-Package==1.0\n");

    auto result = analyzer.analyze(tmp_dir);
    EXPECT_TRUE(hasDep(result.dependencies, "pillow"));
    EXPECT_TRUE(hasDep(result.dependencies, "some_package"));
}

TEST_F(PythonAnalyzerTest, ExtractsVersion) {
    writeFile("requirements.txt", "Django==4.2.1\n");

    auto result = analyzer.analyze(tmp_dir);
    auto it = std::find_if(result.dependencies.begin(), result.dependencies.end(),
        [](const auto& d) { return d.package_name == "django"; });
    ASSERT_NE(it, result.dependencies.end());
    EXPECT_EQ(it->package_version, "4.2.1");
}

TEST_F(PythonAnalyzerTest, ParsesPyprojectTomlDependencies) {
    writeFile("pyproject.toml",
        "[project]\n"
        "name = \"myapp\"\n"
        "dependencies = [\n"
        "    \"httpx>=0.24\",\n"
        "    \"pydantic==2.0.0\",\n"
        "]\n");

    auto result = analyzer.analyze(tmp_dir);
    EXPECT_TRUE(hasDep(result.dependencies, "httpx"));
    EXPECT_TRUE(hasDep(result.dependencies, "pydantic"));
}
