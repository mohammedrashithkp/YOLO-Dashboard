#pragma once

#include "yolo_dashboard/common.h"
#include <deque>
#include <mutex>
#include <fstream>

namespace yolo_dashboard {

/// Collects and exports inference performance metrics.
/// Tracks FPS, latency, detection counts with rolling windows.
class MetricsCollector {
public:
    MetricsCollector();
    ~MetricsCollector() = default;

    /// Record a new detection result (called after each inference).
    void recordResult(const DetectionResult& result);

    /// Get current metrics snapshot.
    MetricsSnapshot getSnapshot() const;

    /// Get metrics history (last N snapshots).
    std::vector<MetricsSnapshot> getHistory(int count = 60) const;

    /// Export metrics to a JSON file.
    bool exportToJson(const std::string& filepath) const;

    /// Export metrics to a CSV file.
    bool exportToCsv(const std::string& filepath) const;

    /// Reset all metrics.
    void reset();

    /// Get total frame count.
    int getTotalFrames() const { return total_frames_; }

private:
    void updateSnapshot();

    mutable std::mutex mutex_;
    std::deque<DetectionResult> recent_results_; // Last N results for rolling average
    std::deque<MetricsSnapshot> history_;         // Historical snapshots

    std::atomic<int> total_frames_{0};
    std::atomic<int> total_detections_{0};

    TimePoint last_snapshot_time_;
    TimePoint session_start_;

    static constexpr int MAX_RECENT_RESULTS = 300;  // ~10 seconds at 30fps
    static constexpr int MAX_HISTORY = 3600;         // 1 hour at 1/sec
};

} // namespace yolo_dashboard
