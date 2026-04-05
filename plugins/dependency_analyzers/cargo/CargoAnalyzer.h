#pragma once
#include <interfaces/IDependencyAnalyzer.h>

namespace blueduck::plugins::cargo {

class CargoAnalyzer : public IDependencyAnalyzer {
public:
    AnalysisResult analyze(const std::string& repo_path) override;
    std::string name() const override { return "cargo"; }
    std::string ecosystem() const override { return "cargo"; }
    bool canAnalyze(const std::string& repo_path) const override;
};

} // namespace blueduck::plugins::cargo
