#pragma once
#include "PluginMeta.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace blueduck {

struct Dependency {
    std::string ecosystem;
    std::string package_name;
    std::string package_version;
};

struct AnalysisResult {
    bool                    success       = false;
    std::string             error_message;
    std::vector<Dependency> dependencies;
};

class IDependencyAnalyzer {
public:
    virtual ~IDependencyAnalyzer() = default;

    /// Return a unique machine-readable name, e.g. "cmake_cpp", "npm".
    virtual std::string name() const = 0;

    /// Return the ecosystem string written to the database, e.g. "cmake", "pypi".
    virtual std::string ecosystem() const = 0;

    /// Called once after loading; passes db_connstr and other config.
    virtual void configure(const std::unordered_map<std::string, std::string>& config) {}

    /// Return true if this analyzer can handle the given repository path.
    virtual bool canAnalyze(const std::string& repo_path) const = 0;

    /// Analyze the repository at repo_path and return found dependencies.
    virtual AnalysisResult analyze(const std::string& repo_path) = 0;
};

} // namespace blueduck
