#pragma once
#include <interfaces/IDependencyAnalyzer.h>

namespace blueduck::plugins::nuget {

/// Analyzes .NET projects by parsing *.csproj / *.fsproj / *.vbproj files
/// for PackageReference elements, and packages.config files.
class NugetAnalyzer : public IDependencyAnalyzer {
public:
    AnalysisResult analyze(const std::string& repo_path) override;
    std::string name() const override { return "nuget"; }
    std::string ecosystem() const override { return "nuget"; }
    bool canAnalyze(const std::string& repo_path) const override;

private:
    void parseProjectFile(const std::string& path,
                           std::vector<Dependency>& deps) const;
    void parsePackagesConfig(const std::string& path,
                              std::vector<Dependency>& deps) const;
};

} // namespace blueduck::plugins::nuget
