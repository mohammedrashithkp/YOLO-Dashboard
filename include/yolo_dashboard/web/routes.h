#pragma once

#include "yolo_dashboard/common.h"
#include "yolo_dashboard/camera/camera_manager.h"
#include "yolo_dashboard/camera/video_recorder.h"
#include "yolo_dashboard/hardware/hardware_detector.h"
#include "yolo_dashboard/inference/inference_engine.h"
#include "yolo_dashboard/config/config_manager.h"
#include "yolo_dashboard/auth/auth_manager.h"
#include "yolo_dashboard/metrics/metrics_collector.h"
#include "yolo_dashboard/metrics/log_manager.h"

#include "crow.h"

namespace yolo_dashboard {

/// Registers all REST API routes on the Crow application.
class RouteHandler {
public:
    RouteHandler(CameraManager& camera, VideoRecorder& recorder,
                 ConfigManager& config, AuthManager& auth,
                 MetricsCollector& metrics, LogManager& logger);

    /// Register all API routes on the given Crow app.
    void registerRoutes(crow::SimpleApp& app);

private:
    // ── Auth middleware ──
    bool checkAuth(const crow::request& req, crow::response& res);

    // ── Route handlers ──
    // Auth
    crow::response handleLogin(const crow::request& req);
    crow::response handleLogout(const crow::request& req);

    // Camera
    crow::response handleListCameras(const crow::request& req);
    crow::response handleSelectCamera(const crow::request& req);

    // Hardware
    crow::response handleDetectHardware(const crow::request& req);

    // Models
    crow::response handleBrowseModels(const crow::request& req);
    crow::response handleUploadModel(const crow::request& req);

    // Inference
    crow::response handleStartInference(const crow::request& req);
    crow::response handleStopInference(const crow::request& req);
    crow::response handleInferenceStatus(const crow::request& req);

    // Config
    crow::response handleGetConfig(const crow::request& req);
    crow::response handleUpdateConfig(const crow::request& req);
    crow::response handleDownloadConfig(const crow::request& req);
    crow::response handleUploadConfig(const crow::request& req);

    // Metrics
    crow::response handleGetMetrics(const crow::request& req);
    crow::response handleDownloadMetrics(const crow::request& req);

    // Logs
    crow::response handleGetLogs(const crow::request& req);
    crow::response handleDownloadLogs(const crow::request& req);
    crow::response handleSetLogLevel(const crow::request& req);

    // Recording
    crow::response handleStartRecording(const crow::request& req);
    crow::response handleStopRecording(const crow::request& req);
    crow::response handleListRecordings(const crow::request& req);

    // Stream
    void handleMjpegStream(const crow::request& req, crow::response& res);

    // File browser helpers
    std::vector<FileEntry> browseDirectory(const std::string& path,
                                            const std::vector<std::string>& extensions);

    // References to subsystems
    CameraManager& camera_;
    VideoRecorder& recorder_;
    ConfigManager& config_;
    AuthManager& auth_;
    MetricsCollector& metrics_;
    LogManager& logger_;

    // Inference engine (created on demand based on mode selection)
    std::unique_ptr<InferenceEngine> inference_engine_;
    std::atomic<bool> inference_running_{false};
    std::thread inference_thread_;
    std::mutex inference_mutex_;
};

} // namespace yolo_dashboard
