#pragma once

#include "yolo_dashboard/camera/camera_manager.h"
#include "yolo_dashboard/camera/video_recorder.h"
#include "yolo_dashboard/hardware/hardware_detector.h"
#include "yolo_dashboard/inference/inference_engine.h"
#include "yolo_dashboard/config/config_manager.h"
#include "yolo_dashboard/auth/auth_manager.h"
#include "yolo_dashboard/metrics/metrics_collector.h"
#include "yolo_dashboard/metrics/log_manager.h"
#include "yolo_dashboard/web/routes.h"

#include "crow.h"
#include <memory>

namespace yolo_dashboard {

/// Main application class — initializes all subsystems and runs the web server.
class Application {
public:
    Application();
    ~Application();

    /// Initialize the application with a config file path.
    bool initialize(const std::string& config_path);

    /// Run the web server (blocking).
    void run();

    /// Stop the application gracefully.
    void stop();

private:
    /// Set up directories for data storage.
    void ensureDirectories();

    crow::SimpleApp app_;

    // Subsystems
    std::unique_ptr<CameraManager> camera_;
    std::unique_ptr<VideoRecorder> recorder_;
    std::unique_ptr<ConfigManager> config_;
    std::unique_ptr<AuthManager> auth_;
    std::unique_ptr<MetricsCollector> metrics_;
    std::unique_ptr<LogManager> logger_;
    std::unique_ptr<RouteHandler> routes_;
};

} // namespace yolo_dashboard
