/// YOLO Dashboard — Main Entry Point
/// Parses CLI arguments, initializes logging, and starts the application.

#include "yolo_dashboard/app.h"
#include <iostream>
#include <string>
#include <csignal>

static yolo_dashboard::Application* g_app = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[YOLO Dashboard] Received signal " << signum << ", shutting down..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

int main(int argc, char* argv[]) {
    std::string config_path = "config/default_config.yaml";

    // Parse simple CLI args
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "YOLO Dashboard — AI Inference Management Platform\n"
                      << "\nUsage: yolo_dashboard [options]\n"
                      << "\nOptions:\n"
                      << "  -c, --config <path>  Path to YAML config file (default: config/default_config.yaml)\n"
                      << "  -h, --help           Show this help message\n"
                      << std::endl;
            return 0;
        }
    }

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << R"(
  __   __  ___   _     ___    ___          _     _                        _
  \ \ / / / _ \ | |   / _ \  |   \  __ _ _| |_  | |__  ___  __ _  _ _  __| |
   \ V / | (_) || |__| (_) | | |) |/ _` (_-< '_ \| '_ \/ _ \/ _` || '_|/ _` |
    |_|   \___/ |____|\___/  |___/ \__,_/__/_.__/|_.__/\___/\__,_||_|  \__,_|
    )" << "\n  v1.0.0 — AI Inference Management Platform\n" << std::endl;

    yolo_dashboard::Application app;
    g_app = &app;

    if (!app.initialize(config_path)) {
        std::cerr << "[ERROR] Failed to initialize application with config: " << config_path << std::endl;
        return 1;
    }

    app.run();
    return 0;
}
