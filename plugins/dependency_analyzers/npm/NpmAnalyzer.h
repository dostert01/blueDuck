#pragma once
#include <interfaces/IDependencyAnalyzer.h>

namespace blueduck::plugins::npm {

class NpmAnalyzer : public IDependencyAnalyzer {
public:
    AnalysisResult analyze(const std::string& repo_path) override;
    std::string name() const override { return "npm"; }
    std::string ecosystem() const override { return "npm"; }
    bool canAnalyze(const std::string& repo_path) const override;
};

} // namespace blueduck::plugins::npm
