#pragma once

#include "yolo_dashboard/common.h"
#include "yolo_dashboard/camera/camera_manager.h"
#include "yolo_dashboard/inference/inference_engine.h"
#include "yolo_dashboard/metrics/metrics_collector.h"
#include "crow.h"
#include <mutex>
#include <unordered_set>

namespace yolo_dashboard {

class WebSocketHandler {
public:
    WebSocketHandler(CameraManager& camera, MetricsCollector& metrics);
    ~WebSocketHandler();

    void registerRoutes(crow::SimpleApp& app);

    // Call this repeatedly from a background thread or event loop
    // to broadcast the latest frame and metrics
    void broadcastUpdate();

    void setInferenceEngine(InferenceEngine* engine) { inference_engine_ = engine; }
    void setInferenceRunning(bool running) { inference_running_ = running; }

private:
    void broadcastFrame();
    void broadcastMetrics();

    CameraManager& camera_;
    MetricsCollector& metrics_;
    
    InferenceEngine* inference_engine_ = nullptr;
    bool inference_running_ = false;

    std::mutex stream_clients_mutex_;
    std::unordered_set<crow::websocket::connection*> stream_clients_;

    std::mutex metrics_clients_mutex_;
    std::unordered_set<crow::websocket::connection*> metrics_clients_;
};

} // namespace yolo_dashboard
