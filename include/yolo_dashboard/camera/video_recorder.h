#pragma once

#include "yolo_dashboard/common.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <condition_variable>

namespace yolo_dashboard {

/// Records camera frames to MP4 files with H.264 encoding.
/// Supports both raw and annotated (with bounding boxes) frame recording.
class VideoRecorder {
public:
    VideoRecorder();
    ~VideoRecorder();

    /// Start recording to the specified output directory.
    /// Returns the full path of the output file.
    std::string startRecording(const std::string& output_dir,
                                Resolution resolution,
                                double fps = 30.0,
                                const std::string& codec = "avc1");

    /// Stop the current recording and finalize the file.
    RecordingInfo stopRecording();

    /// Queue a frame for writing (thread-safe, non-blocking).
    void writeFrame(const cv::Mat& frame);

    /// Check if currently recording.
    bool isRecording() const;

    /// Get info about the current recording.
    RecordingInfo getCurrentRecordingInfo() const;

    /// List all saved recordings in a directory.
    static std::vector<RecordingInfo> listRecordings(const std::string& directory);

private:
    void writerLoop();

    cv::VideoWriter writer_;
    std::queue<cv::Mat> frame_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread writer_thread_;
    std::atomic<bool> recording_{false};

    std::string current_filepath_;
    TimePoint recording_start_;
    int frame_count_ = 0;
    Resolution current_resolution_;
};

} // namespace yolo_dashboard
