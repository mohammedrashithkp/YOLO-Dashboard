#include "yolo_dashboard/camera/camera_manager.h"
#include "yolo_dashboard/camera/camera_enumerator.h"
#include <iostream>
#include <chrono>

namespace yolo_dashboard {

CameraManager::CameraManager() = default;

CameraManager::~CameraManager() {
    closeCamera();
}

std::vector<CameraInfo> CameraManager::listCameras() {
    std::lock_guard<std::mutex> lock(cameras_mutex_);
    cached_cameras_ = CameraEnumerator::enumerate();
    return cached_cameras_;
}

bool CameraManager::openCamera(int device_id, Resolution res, int fps) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    closeCamera(); // Close any existing camera first

    target_fps_ = fps;
    current_resolution_ = res;

#ifdef _WIN32
    int backend = cv::CAP_MSMF;    // Media Foundation on Windows
#else
    int backend = cv::CAP_V4L2;    // V4L2 on Linux
#endif

    if (!capture_.open(device_id, backend)) {
        // Fallback to auto-detect backend
        if (!capture_.open(device_id, cv::CAP_ANY)) {
            std::cerr << "[Camera] Failed to open camera " << device_id << std::endl;
            return false;
        }
    }

    // Set capture properties
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, res.width);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, res.height);
    capture_.set(cv::CAP_PROP_FPS, fps);

    // Read actual values (camera may not support requested settings)
    current_resolution_.width = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
    current_resolution_.height = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));

    current_camera_.id = device_id;
    current_camera_.is_available = true;

    // Start capture thread
    running_ = true;
    capture_thread_ = std::thread(&CameraManager::captureLoop, this);

    std::cout << "[Camera] Opened camera " << device_id
              << " at " << current_resolution_.width << "x" << current_resolution_.height
              << std::endl;
    return true;
}

void CameraManager::closeCamera() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    if (capture_.isOpened()) {
        capture_.release();
    }
    current_camera_ = CameraInfo{};
}

bool CameraManager::getLatestFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (latest_frame_.empty()) return false;
    latest_frame_.copyTo(frame);
    return true;
}

bool CameraManager::getLatestJpeg(std::vector<uint8_t>& jpeg_buffer, int quality) {
    cv::Mat frame;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if (latest_frame_.empty()) return false;
        latest_frame_.copyTo(frame);
    }

    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
    return cv::imencode(".jpg", frame, jpeg_buffer, params);
}

bool CameraManager::isOpen() const {
    return running_ && capture_.isOpened();
}

CameraInfo CameraManager::getCurrentCamera() const {
    return current_camera_;
}

Resolution CameraManager::getResolution() const {
    return current_resolution_;
}

double CameraManager::getActualFps() const {
    return actual_fps_.load();
}

void CameraManager::captureLoop() {
    cv::Mat frame;
    auto last_time = Clock::now();
    int frame_count = 0;

    while (running_) {
        if (!capture_.read(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            frame.copyTo(latest_frame_);
        }

        // Calculate FPS
        frame_count++;
        auto now = Clock::now();
        auto elapsed = std::chrono::duration<double>(now - last_time).count();
        if (elapsed >= 1.0) {
            actual_fps_ = frame_count / elapsed;
            frame_count = 0;
            last_time = now;
        }

        // Throttle to target FPS
        if (target_fps_ > 0) {
            auto frame_time = std::chrono::microseconds(1000000 / target_fps_);
            std::this_thread::sleep_for(frame_time);
        }
    }
}

} // namespace yolo_dashboard
