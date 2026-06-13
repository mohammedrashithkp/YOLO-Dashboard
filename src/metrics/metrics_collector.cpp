#include "yolo_dashboard/metrics/metrics_collector.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>

namespace yolo_dashboard {

MetricsCollector::MetricsCollector() {
    session_start_ = Clock::now();
    last_snapshot_time_ = Clock::now();
}

void MetricsCollector::recordResult(const DetectionResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);

    recent_results_.push_back(result);
    if (recent_results_.size() > MAX_RECENT_RESULTS) {
        recent_results_.pop_front();
    }

    total_frames_++;
    total_detections_ += static_cast<int>(result.detections.size());

    // Update snapshot every second
    auto now = Clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_snapshot_time_).count();
    if (elapsed >= 1.0) {
        updateSnapshot();
        last_snapshot_time_ = now;
    }
}

MetricsSnapshot MetricsCollector::getSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    MetricsSnapshot snapshot;

    if (recent_results_.empty()) return snapshot;

    // Calculate rolling averages from recent results
    double total_inference = 0, total_preprocess = 0, total_postprocess = 0;
    int total_dets = 0;

    // Use last 30 frames for rolling average
    int count = 0;
    for (auto it = recent_results_.rbegin();
         it != recent_results_.rend() && count < 30; ++it, ++count) {
        total_inference += it->inference_time_ms;
        total_preprocess += it->preprocess_time_ms;
        total_postprocess += it->postprocess_time_ms;
        total_dets += static_cast<int>(it->detections.size());
    }

    if (count > 0) {
        snapshot.avg_inference_ms = total_inference / count;
        snapshot.avg_preprocess_ms = total_preprocess / count;
        snapshot.avg_postprocess_ms = total_postprocess / count;
        snapshot.total_detections = total_dets;

        // FPS from total processing time
        double avg_total_ms = snapshot.avg_inference_ms + snapshot.avg_preprocess_ms + snapshot.avg_postprocess_ms;
        snapshot.fps = avg_total_ms > 0 ? 1000.0 / avg_total_ms : 0.0;
    }

    snapshot.total_frames = total_frames_.load();

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream ts;
    ts << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    snapshot.timestamp = ts.str();

    return snapshot;
}

std::vector<MetricsSnapshot> MetricsCollector::getHistory(int count) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MetricsSnapshot> result;
    int start = std::max(0, static_cast<int>(history_.size()) - count);
    for (int i = start; i < static_cast<int>(history_.size()); ++i) {
        result.push_back(history_[i]);
    }
    return result;
}

bool MetricsCollector::exportToJson(const std::string& filepath) const {
    auto history = getHistory(MAX_HISTORY);

    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "{\n  \"session_start\": \"" << getSnapshot().timestamp << "\",\n";
    out << "  \"total_frames\": " << total_frames_.load() << ",\n";
    out << "  \"total_detections\": " << total_detections_.load() << ",\n";
    out << "  \"snapshots\": [\n";

    for (size_t i = 0; i < history.size(); ++i) {
        const auto& s = history[i];
        out << "    {\n";
        out << "      \"timestamp\": \"" << s.timestamp << "\",\n";
        out << "      \"fps\": " << std::fixed << std::setprecision(2) << s.fps << ",\n";
        out << "      \"avg_inference_ms\": " << s.avg_inference_ms << ",\n";
        out << "      \"avg_preprocess_ms\": " << s.avg_preprocess_ms << ",\n";
        out << "      \"avg_postprocess_ms\": " << s.avg_postprocess_ms << ",\n";
        out << "      \"total_detections\": " << s.total_detections << ",\n";
        out << "      \"total_frames\": " << s.total_frames << "\n";
        out << "    }";
        if (i + 1 < history.size()) out << ",";
        out << "\n";
    }

    out << "  ]\n}\n";
    return true;
}

bool MetricsCollector::exportToCsv(const std::string& filepath) const {
    auto history = getHistory(MAX_HISTORY);

    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "timestamp,fps,avg_inference_ms,avg_preprocess_ms,avg_postprocess_ms,total_detections,total_frames\n";

    for (const auto& s : history) {
        out << s.timestamp << ","
            << std::fixed << std::setprecision(2)
            << s.fps << ","
            << s.avg_inference_ms << ","
            << s.avg_preprocess_ms << ","
            << s.avg_postprocess_ms << ","
            << s.total_detections << ","
            << s.total_frames << "\n";
    }

    return true;
}

void MetricsCollector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    recent_results_.clear();
    history_.clear();
    total_frames_ = 0;
    total_detections_ = 0;
    session_start_ = Clock::now();
    last_snapshot_time_ = Clock::now();
}

void MetricsCollector::updateSnapshot() {
    // Called with mutex held
    MetricsSnapshot snapshot;

    if (!recent_results_.empty()) {
        double total_inference = 0, total_preprocess = 0, total_postprocess = 0;
        int total_dets = 0;
        int count = 0;

        for (auto it = recent_results_.rbegin();
             it != recent_results_.rend() && count < 30; ++it, ++count) {
            total_inference += it->inference_time_ms;
            total_preprocess += it->preprocess_time_ms;
            total_postprocess += it->postprocess_time_ms;
            total_dets += static_cast<int>(it->detections.size());
        }

        if (count > 0) {
            snapshot.avg_inference_ms = total_inference / count;
            snapshot.avg_preprocess_ms = total_preprocess / count;
            snapshot.avg_postprocess_ms = total_postprocess / count;
            snapshot.total_detections = total_dets;
            double avg_total = snapshot.avg_inference_ms + snapshot.avg_preprocess_ms + snapshot.avg_postprocess_ms;
            snapshot.fps = avg_total > 0 ? 1000.0 / avg_total : 0.0;
        }
    }

    snapshot.total_frames = total_frames_.load();

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream ts;
    ts << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    snapshot.timestamp = ts.str();

    history_.push_back(snapshot);
    if (history_.size() > MAX_HISTORY) {
        history_.pop_front();
    }
}

} // namespace yolo_dashboard
