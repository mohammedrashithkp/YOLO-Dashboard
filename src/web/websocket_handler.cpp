#define CROW_ENFORCE_WS_SPEC
#include "yolo_dashboard/web/websocket_handler.h"
#include <opencv2/opencv.hpp>
#include <iostream>

namespace yolo_dashboard {

WebSocketHandler::WebSocketHandler(CameraManager& camera, MetricsCollector& metrics)
    : camera_(camera), metrics_(metrics) {}

WebSocketHandler::~WebSocketHandler() {
    // Crow handles connection cleanup, but we can clear our sets
    std::lock_guard<std::mutex> lock1(stream_clients_mutex_);
    stream_clients_.clear();
    
    std::lock_guard<std::mutex> lock2(metrics_clients_mutex_);
    metrics_clients_.clear();
}

void WebSocketHandler::registerRoutes(crow::SimpleApp& app) {
    // Live Camera Stream WebSocket
    CROW_ROUTE(app, "/ws/stream")
        .websocket(&app)
        .onopen([&](crow::websocket::connection& conn) {
            std::lock_guard<std::mutex> lock(stream_clients_mutex_);
            stream_clients_.insert(&conn);
            std::cout << "[WebSocket] Stream client connected" << std::endl;
        })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            std::lock_guard<std::mutex> lock(stream_clients_mutex_);
            stream_clients_.erase(&conn);
            std::cout << "[WebSocket] Stream client disconnected: " << reason << std::endl;
        })
        .onmessage([&](crow::websocket::connection& /*conn*/, const std::string& /*data*/, bool /*is_binary*/) {
            // Can handle client commands if needed
        });

    // Live Metrics WebSocket
    CROW_ROUTE(app, "/ws/metrics")
        .websocket(&app)
        .onopen([&](crow::websocket::connection& conn) {
            std::lock_guard<std::mutex> lock(metrics_clients_mutex_);
            metrics_clients_.insert(&conn);
        })
        .onclose([&](crow::websocket::connection& conn, const std::string& /*reason*/) {
            std::lock_guard<std::mutex> lock(metrics_clients_mutex_);
            metrics_clients_.erase(&conn);
        });
}

void WebSocketHandler::broadcastUpdate() {
    broadcastFrame();
    broadcastMetrics();
}

void WebSocketHandler::broadcastFrame() {
    std::vector<crow::websocket::connection*> clients_copy;
    {
        std::lock_guard<std::mutex> lock(stream_clients_mutex_);
        if (stream_clients_.empty()) return;
        for (auto* conn : stream_clients_) {
            clients_copy.push_back(conn);
        }
    }

    if (!camera_.isOpen()) return;

    cv::Mat frame;
    if (!camera_.getLatestFrame(frame)) return;

    // Run inference if active
    if (inference_running_ && inference_engine_ && inference_engine_->isModelLoaded()) {
        DetectionResult result = inference_engine_->infer(frame);
        
        // Draw bounding boxes on frame
        for (const auto& det : result.detections) {
            cv::rectangle(frame, cv::Point(det.x1, det.y1), cv::Point(det.x2, det.y2), cv::Scalar(0, 255, 0), 2);
            std::string label = det.class_name + " " + std::to_string(det.confidence).substr(0, 4);
            int baseLine;
            cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
            cv::rectangle(frame, cv::Point(det.x1, det.y1 - labelSize.height),
                          cv::Point(det.x1 + labelSize.width, det.y1 + baseLine),
                          cv::Scalar(0, 255, 0), cv::FILLED);
            cv::putText(frame, label, cv::Point(det.x1, det.y1),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
        }
        
        metrics_.recordResult(result);
    }

    std::vector<uint8_t> buf;
    cv::imencode(".jpg", frame, buf, {cv::IMWRITE_JPEG_QUALITY, 80});
    std::string jpeg_data(buf.begin(), buf.end());

    for (auto* conn : clients_copy) {
        conn->send_binary(jpeg_data);
    }
}

void WebSocketHandler::broadcastMetrics() {
    std::vector<crow::websocket::connection*> clients_copy;
    {
        std::lock_guard<std::mutex> lock(metrics_clients_mutex_);
        if (metrics_clients_.empty()) return;
        for (auto* conn : metrics_clients_) {
            clients_copy.push_back(conn);
        }
    }

    auto snapshot = metrics_.getSnapshot();
    
    crow::json::wvalue j;
    j["fps"] = snapshot.fps;
    j["inference_ms"] = snapshot.avg_inference_ms;
    j["total_detections"] = snapshot.total_detections;
    
    std::string metrics_str = j.dump();
    for (auto* conn : clients_copy) {
        conn->send_text(metrics_str);
    }
}

} // namespace yolo_dashboard
