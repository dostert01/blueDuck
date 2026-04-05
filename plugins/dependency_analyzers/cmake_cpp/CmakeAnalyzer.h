#pragma once
#include <interfaces/IDependencyAnalyzer.h>
#include <string>

namespace blueduck::plugins::cmake_cpp {

/// Analyzes C++ projects by scanning CMakeLists.txt files for
/// find_package() and FetchContent_Declare() calls.
class CmakeAnalyzer : public IDependencyAnalyzer {
public:
    AnalysisResult analyze(const std::string& repo_path) override;
    std::string name() const override { return "cmake_cpp"; }
    std::string ecosystem() const override { return "cmake"; }
    bool canAnalyze(const std::string& repo_path) const override;

private:
    void scanDirectory(const std::string& dir,
                       std::vector<Dependency>& deps) const;
    void parseFile(const std::string& file_path,
                   std::vector<Dependency>& deps) const;
};

} // namespace blueduck::plugins::cmake_cpp
