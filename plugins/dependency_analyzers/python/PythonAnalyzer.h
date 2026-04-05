#pragma once
#include <interfaces/IDependencyAnalyzer.h>

namespace blueduck::plugins::python {

/// Analyzes Python projects by reading requirements.txt, setup.cfg,
/// pyproject.toml (PEP 517/518) and setup.py (static install_requires only).
class PythonAnalyzer : public IDependencyAnalyzer {
public:
    AnalysisResult analyze(const std::string& repo_path) override;
    std::string name() const override { return "python"; }
    std::string ecosystem() const override { return "pypi"; }
    bool canAnalyze(const std::string& repo_path) const override;

private:
    void parseRequirementsTxt(const std::string& path,
                               std::vector<Dependency>& deps) const;
    void parsePyprojectToml(const std::string& path,
                             std::vector<Dependency>& deps) const;
    void parseSetupCfg(const std::string& path,
                        std::vector<Dependency>& deps) const;

    static Dependency depFromLine(const std::string& line);
};

} // namespace blueduck::plugins::python
