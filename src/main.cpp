#include <core/Application.h>
#include <iostream>
#include <string>

static void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " --config <path/to/config.json>\n"
              << "\n"
              << "Environment variables:\n"
              << "  BLUEDUCK_SECRET_KEY   32-byte hex key for credential encryption (required)\n"
              << "  BD_DB_HOST            PostgreSQL host       (overrides config)\n"
              << "  BD_DB_PORT            PostgreSQL port       (overrides config)\n"
              << "  BD_DB_NAME            PostgreSQL database   (overrides config)\n"
              << "  BD_DB_USER            PostgreSQL user       (overrides config)\n"
              << "  BD_DB_PASSWORD        PostgreSQL password   (overrides config)\n"
              << "  BD_NVD_API_KEY        NVD API key           (overrides config)\n";
}

int main(int argc, char* argv[]) {
    std::string config_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if ((arg == "--config" || arg == "-c") && i + 1 < argc)
            config_path = argv[++i];
        else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (config_path.empty()) {
        std::cerr << "Error: --config is required\n\n";
        printUsage(argv[0]);
        return 1;
    }

    try {
        auto& app = blueduck::Application::instance();
        app.init(config_path);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[blueDuck] Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
