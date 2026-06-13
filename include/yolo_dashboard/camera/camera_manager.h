#pragma once

#include "yolo_dashboard/common.h"
#include <opencv2/opencv.hpp>
#include <thread>

namespace yolo_dashboard {

/// Manages camera discovery, selection, and frame capture.
/// Runs a background capture thread and distributes frames to registered consumers.
class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    /// Scan for available cameras using platform-specific enumerator.
    std::vector<CameraInfo> listCameras();

    /// Open a camera by device ID with specified settings.
    bool openCamera(int device_id, Resolution res = {640, 480}, int fps = 30);

    /// Close the currently active camera.
    void closeCamera();

    /// Get the latest captured frame (thread-safe).
    bool getLatestFrame(cv::Mat& frame);

    /// Get the latest frame encoded as JPEG (for streaming).
    bool getLatestJpeg(std::vector<uint8_t>& jpeg_buffer, int quality = 80);

    /// Check if a camera is currently open and capturing.
    bool isOpen() const;

    /// Get info about the currently active camera.
    CameraInfo getCurrentCamera() const;

    /// Get current resolution.
    Resolution getResolution() const;

    /// Get current FPS.
    double getActualFps() const;

private:
    void captureLoop();

    cv::VideoCapture capture_;
    cv::Mat latest_frame_;
    mutable std::mutex frame_mutex_;
    std::thread capture_thread_;
    std::atomic<bool> running_{false};
    std::atomic<double> actual_fps_{0.0};

    CameraInfo current_camera_;
    Resolution current_resolution_;
    int target_fps_ = 30;

    std::vector<CameraInfo> cached_cameras_;
    mutable std::mutex cameras_mutex_;
    mutable std::recursive_mutex state_mutex_;
};

} // namespace yolo_dashboard
