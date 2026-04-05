#pragma once
#include <cstdint>

#define BLUEDUCK_PLUGIN_API_VERSION 1u

namespace blueduck {

struct PluginMeta {
    uint32_t    api_version;    ///< Must equal BLUEDUCK_PLUGIN_API_VERSION
    const char* plugin_type;    ///< "cve_source" | "dependency_analyzer"
    const char* plugin_name;    ///< unique machine-readable id: "nvd", "npm", ...
    const char* plugin_version; ///< semver string, e.g. "1.0.0"
};

} // namespace blueduck

/// Convenience macro — optional alternative to writing extern "C" symbols by hand.
/// Usage: place at file scope in a plugin .cpp:
///
///   BLUEDUCK_REGISTER_PLUGIN(NvdSource, "cve_source", "nvd", "1.0.0")
///
#define BLUEDUCK_REGISTER_PLUGIN(ClassName, TypeStr, NameStr, VerStr)       \
    namespace {                                                              \
        static blueduck::PluginMeta _bd_meta = {                            \
            BLUEDUCK_PLUGIN_API_VERSION, TypeStr, NameStr, VerStr           \
        };                                                                   \
    }                                                                        \
    extern "C" {                                                             \
        const blueduck::PluginMeta* blueduck_plugin_meta() {                \
            return &_bd_meta;                                                \
        }                                                                    \
        void* blueduck_create_plugin() { return new ClassName(); }          \
        void  blueduck_destroy_plugin(void* p) {                            \
            delete static_cast<ClassName*>(p);                               \
        }                                                                    \
    }
