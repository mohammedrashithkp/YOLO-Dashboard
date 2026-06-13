#include "yolo_dashboard/app.h"
#include <iostream>
#include <filesystem>

namespace yolo_dashboard {
namespace fs = std::filesystem;

Application::Application() = default;

Application::~Application() {
    stop();
}

bool Application::initialize(const std::string& config_path) {
    // 1. Initialize config
    config_ = std::make_unique<ConfigManager>();
    if (fs::exists(config_path)) {
        if (!config_->loadFromFile(config_path)) {
            std::cerr << "[WARN] Failed to load config from " << config_path
                      << ", using defaults" << std::endl;
            config_->resetToDefaults();
        }
    } else {
        std::cout << "[INFO] Config file not found at " << config_path
                  << ", using defaults" << std::endl;
        config_->resetToDefaults();
    }

    const auto& cfg = config_->getConfig();

    // 2. Initialize logger
    logger_ = std::make_unique<LogManager>();
    ensureDirectories();
    logger_->initialize(cfg.log_file, cfg.log_level, cfg.log_max_size_mb, cfg.log_max_files);
    logger_->info("YOLO Dashboard starting...", "app");

    // 3. Initialize auth
    auth_ = std::make_unique<AuthManager>();
    if (!cfg.auth_password_hash.empty()) {
        auth_->setPasswordHash(cfg.auth_password_hash);
        auth_->configure(cfg.auth_username, "", cfg.auth_token_ttl_hours);
    } else {
        // Default password: "admin" — user should change this
        auth_->configure(cfg.auth_username, "admin", cfg.auth_token_ttl_hours);
        logger_->warn("Using default credentials (admin/admin). Please change the password!", "auth");
    }

    // 4. Initialize camera manager
    camera_ = std::make_unique<CameraManager>();
    logger_->info("Camera manager initialized", "camera");

    // 5. Initialize video recorder
    recorder_ = std::make_unique<VideoRecorder>();
    logger_->info("Video recorder initialized", "recording");

    // 6. Initialize metrics collector
    metrics_ = std::make_unique<MetricsCollector>();
    logger_->info("Metrics collector initialized", "metrics");

    // 7. Set up web routes
    routes_ = std::make_unique<RouteHandler>(
        *camera_, *recorder_, *config_, *auth_, *metrics_, *logger_
    );
    routes_->registerRoutes(app_);

    // 8. Serve static web files
    // Crow serves files from the "web" directory for the SPA
    // We set up a catch-all route for SPA client-side routing
    auto web_dir = fs::current_path() / "web";
    if (!fs::exists(web_dir)) {
        // Try relative to executable
        web_dir = fs::path(fs::current_path()) / "web";
    }

    logger_->info("Serving web UI from: " + web_dir.string(), "web");
    logger_->info("Application initialized successfully", "app");
    logger_->info("Server will start on " + cfg.server_host + ":" + std::to_string(cfg.server_port), "app");

    return true;
}

void Application::run() {
    const auto& cfg = config_->getConfig();

    // Configure Crow
    app_.port(cfg.server_port)
        .bindaddr(cfg.server_host)
        .multithreaded()
        .run();
}

void Application::stop() {
    logger_->info("Shutting down YOLO Dashboard...", "app");
    app_.stop();
}

void Application::ensureDirectories() {
    const auto& cfg = config_->getConfig();

    std::vector<std::string> dirs = {
        cfg.recording_output_dir,
        cfg.metrics_output_dir,
        "data/models",
        "data/logs"
    };

    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) {
            try {
                fs::create_directories(dir);
            } catch (const std::exception& e) {
                std::cerr << "[WARN] Could not create directory " << dir
                          << ": " << e.what() << std::endl;
            }
        }
    }
}

} // namespace yolo_dashboard
