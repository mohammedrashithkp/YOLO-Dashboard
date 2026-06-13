#include "yolo_dashboard/camera/video_recorder.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace yolo_dashboard {

VideoRecorder::VideoRecorder() = default;

VideoRecorder::~VideoRecorder() {
    if (recording_) {
        stopRecording();
    }
}

static std::string generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S");
    return ss.str();
}

static std::string formatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string VideoRecorder::startRecording(const std::string& output_dir,
                                           Resolution resolution,
                                           double fps,
                                           const std::string& codec) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (recording_) {
        std::cerr << "[Recorder] Already recording" << std::endl;
        return current_filepath_;
    }

    // Ensure output directory exists
    fs::create_directories(output_dir);

    // Generate filename with timestamp
    std::string filename = "recording_" + generateTimestamp() + ".mp4";
    current_filepath_ = (fs::path(output_dir) / filename).string();
    current_resolution_ = resolution;

    // Open video writer
    int fourcc = cv::VideoWriter::fourcc(codec[0], codec[1], codec[2], codec[3]);

    if (!writer_.open(current_filepath_, fourcc, fps,
                      cv::Size(resolution.width, resolution.height))) {
        // Fallback to MJPG if H.264 is not available
        std::cerr << "[Recorder] H.264 codec not available, falling back to MJPG" << std::endl;
        fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
        current_filepath_ = (fs::path(output_dir) / ("recording_" + generateTimestamp() + ".avi")).string();

        if (!writer_.open(current_filepath_, fourcc, fps,
                          cv::Size(resolution.width, resolution.height))) {
            std::cerr << "[Recorder] Failed to open video writer" << std::endl;
            return "";
        }
    }

    recording_start_ = Clock::now();
    frame_count_ = 0;
    recording_ = true;

    // Start writer thread
    writer_thread_ = std::thread(&VideoRecorder::writerLoop, this);

    std::cout << "[Recorder] Recording started: " << current_filepath_ << std::endl;
    return current_filepath_;
}

RecordingInfo VideoRecorder::stopRecording() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    RecordingInfo info;

    if (!recording_) return info;

    recording_ = false;
    queue_cv_.notify_all();

    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }

    writer_.release();

    auto elapsed = std::chrono::duration<double>(Clock::now() - recording_start_).count();

    info.filename = fs::path(current_filepath_).filename().string();
    info.filepath = current_filepath_;
    info.duration_seconds = elapsed;
    info.is_active = false;

    if (fs::exists(current_filepath_)) {
        info.size_bytes = fs::file_size(current_filepath_);
    }

    std::cout << "[Recorder] Recording stopped: " << info.filename
              << " (" << frame_count_ << " frames, "
              << std::fixed << std::setprecision(1) << elapsed << "s)" << std::endl;

    return info;
}

void VideoRecorder::writeFrame(const cv::Mat& frame) {
    if (!recording_) return;

    std::lock_guard<std::mutex> lock(queue_mutex_);
    // Limit queue size to prevent memory issues
    if (frame_queue_.size() < 300) {
        frame_queue_.push(frame.clone());
        queue_cv_.notify_one();
    }
}

bool VideoRecorder::isRecording() const {
    return recording_.load();
}

RecordingInfo VideoRecorder::getCurrentRecordingInfo() const {
    RecordingInfo info;
    if (!recording_) return info;

    info.filename = fs::path(current_filepath_).filename().string();
    info.filepath = current_filepath_;
    info.duration_seconds = std::chrono::duration<double>(Clock::now() - recording_start_).count();
    info.is_active = true;

    return info;
}

std::vector<RecordingInfo> VideoRecorder::listRecordings(const std::string& directory) {
    std::vector<RecordingInfo> recordings;

    if (!fs::exists(directory)) return recordings;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mp4" || ext == ".avi" || ext == ".mkv") {
            RecordingInfo info;
            info.filename = entry.path().filename().string();
            info.filepath = entry.path().string();
            info.size_bytes = entry.file_size();
            info.is_active = false;
            
            cv::VideoCapture cap(info.filepath);
            if (cap.isOpened()) {
                double fps = cap.get(cv::CAP_PROP_FPS);
                double frames = cap.get(cv::CAP_PROP_FRAME_COUNT);
                if (fps > 0) {
                    info.duration_seconds = frames / fps;
                }
            }
            
            recordings.push_back(info);
        }
    }

    // Sort by filename (which contains timestamp)
    std::sort(recordings.begin(), recordings.end(),
              [](const RecordingInfo& a, const RecordingInfo& b) {
                  return a.filename > b.filename; // Newest first
              });

    return recordings;
}

void VideoRecorder::writerLoop() {
    while (recording_ || !frame_queue_.empty()) {
        cv::Mat frame;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(100),
                               [this] { return !frame_queue_.empty() || !recording_; });

            if (frame_queue_.empty()) continue;

            frame = frame_queue_.front();
            frame_queue_.pop();
        }

        if (!frame.empty() && writer_.isOpened()) {
            // Resize if needed
            if (frame.cols != current_resolution_.width ||
                frame.rows != current_resolution_.height) {
                cv::resize(frame, frame,
                           cv::Size(current_resolution_.width, current_resolution_.height));
            }
            writer_.write(frame);
            frame_count_++;
        }
    }
}

} // namespace yolo_dashboard
