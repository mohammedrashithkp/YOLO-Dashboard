#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <optional>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>
#include <filesystem>

namespace yolo_dashboard {

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// ── Camera Types ──────────────────────────────────────────────
struct CameraInfo {
    int id = -1;
    std::string name;
    std::string device_path;
    std::string driver;
    bool is_available = false;
};

struct Resolution {
    int width = 640;
    int height = 480;
};

// ── Hardware Types ────────────────────────────────────────────
struct CpuInfo {
    std::string model_name;
    std::string architecture; // "x86_64" or "aarch64"
    int cores = 0;
    int threads = 0;
    double frequency_mhz = 0.0;
};

struct NpuInfo {
    std::string device_id;
    std::string name;           // e.g. "Hailo-8"
    std::string firmware_version;
    double tops = 0.0;          // Tera Operations Per Second
    bool is_available = false;
};

struct MemoryInfo {
    uint64_t total_mb = 0;
    uint64_t available_mb = 0;
};

struct HardwareInfo {
    CpuInfo cpu;
    std::vector<NpuInfo> npus;
    MemoryInfo memory;
};

// ── Inference Types ───────────────────────────────────────────
enum class InferenceMode {
    CPU,
    NPU
};

struct BoundingBox {
    float x1, y1, x2, y2;
    float confidence;
    int class_id;
    std::string class_name;
};

struct DetectionResult {
    std::vector<BoundingBox> detections;
    double inference_time_ms = 0.0;
    double preprocess_time_ms = 0.0;
    double postprocess_time_ms = 0.0;
    int frame_number = 0;
};

struct ModelInfo {
    std::string path;
    std::string name;
    std::string format;          // "onnx" or "hef"
    int64_t file_size_bytes = 0;
    std::vector<int64_t> input_shape;
    std::vector<int64_t> output_shape;
    bool is_loaded = false;
};

// ── Metrics Types ─────────────────────────────────────────────
struct MetricsSnapshot {
    double fps = 0.0;
    double avg_inference_ms = 0.0;
    double avg_preprocess_ms = 0.0;
    double avg_postprocess_ms = 0.0;
    int total_detections = 0;
    int total_frames = 0;
    std::string timestamp;
};

// ── Config Types ──────────────────────────────────────────────
struct AppConfig {
    // Server
    int server_port = 8080;
    std::string server_host = "0.0.0.0";

    // Camera
    int camera_device_id = 0;
    Resolution camera_resolution = {640, 480};
    int camera_fps = 30;

    // Inference
    std::string inference_mode = "cpu";
    std::string model_path;
    float confidence_threshold = 0.5f;
    float nms_threshold = 0.45f;
    int cpu_threads = 4;

    // Recording
    bool recording_enabled = false;
    std::string recording_output_dir = "./data/recordings";
    std::string recording_codec = "avc1";
    bool recording_annotated = true;

    // Logging
    std::string log_level = "info";
    std::string log_file = "./data/logs/dashboard.log";
    int log_max_size_mb = 50;
    int log_max_files = 5;

    // Metrics
    int metrics_export_interval_sec = 60;
    std::string metrics_output_dir = "./data/metrics";
    std::string metrics_format = "json";

    // Auth
    std::string auth_username = "admin";
    std::string auth_password_hash;
    int auth_token_ttl_hours = 8;
};

// ── File Browser Types ────────────────────────────────────────
struct FileEntry {
    std::string name;
    std::string path;
    bool is_directory = false;
    int64_t size_bytes = 0;
    std::string extension;
};

// ── Recording Types ───────────────────────────────────────────
struct RecordingInfo {
    std::string filename;
    std::string filepath;
    std::string start_time;
    std::string end_time;
    int64_t size_bytes = 0;
    double duration_seconds = 0.0;
    bool is_active = false;
};

// ── Auth Types ────────────────────────────────────────────────
struct AuthToken {
    std::string token;
    std::string username;
    TimePoint expires_at;
    bool is_valid() const {
        return Clock::now() < expires_at;
    }
};

} // namespace yolo_dashboard
