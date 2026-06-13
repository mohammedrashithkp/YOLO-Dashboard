#include "yolo_dashboard/metrics/log_manager.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace yolo_dashboard {

struct LogManager::Impl {
    std::shared_ptr<spdlog::logger> logger;
};

LogManager::LogManager() : impl_(std::make_unique<Impl>()) {}

LogManager::~LogManager() {
    if (impl_ && impl_->logger) {
        impl_->logger->flush();
    }
    spdlog::shutdown();
}

void LogManager::initialize(const std::string& log_file, const std::string& level,
                             int max_size_mb, int max_files) {
    log_file_path_ = log_file;
    current_level_ = level;

    // Create parent directory if needed
    auto parent = fs::path(log_file).parent_path();
    if (!parent.empty() && !fs::exists(parent)) {
        fs::create_directories(parent);
    }

    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, max_size_mb * 1024 * 1024, max_files);

        std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};
        impl_->logger = std::make_shared<spdlog::logger>("dashboard", sinks.begin(), sinks.end());

        // Set log level
        setLevel(level);

        // Set pattern: [timestamp] [level] [source] message
        impl_->logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        spdlog::set_default_logger(impl_->logger);
        spdlog::flush_every(std::chrono::seconds(3));

    } catch (const spdlog::spdlog_ex& e) {
        std::cerr << "[LogManager] Logger initialization failed: " << e.what() << std::endl;
    }
}

std::vector<LogEntry> LogManager::getRecentLogs(int count, const std::string& level_filter) const {
    std::lock_guard<std::mutex> lock(log_mutex_);

    std::vector<LogEntry> result;
    result.reserve(std::min(count, static_cast<int>(log_buffer_.size())));

    int collected = 0;
    for (auto it = log_buffer_.rbegin(); it != log_buffer_.rend() && collected < count; ++it) {
        if (level_filter.empty() || it->level == level_filter) {
            result.push_back(*it);
            collected++;
        }
    }

    // Reverse to chronological order
    std::reverse(result.begin(), result.end());
    return result;
}

void LogManager::setLevel(const std::string& level) {
    current_level_ = level;

    if (!impl_->logger) return;

    if (level == "debug") impl_->logger->set_level(spdlog::level::debug);
    else if (level == "info") impl_->logger->set_level(spdlog::level::info);
    else if (level == "warn") impl_->logger->set_level(spdlog::level::warn);
    else if (level == "error") impl_->logger->set_level(spdlog::level::err);
    else impl_->logger->set_level(spdlog::level::info);
}

std::string LogManager::getLevel() const {
    return current_level_;
}

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void LogManager::log(const std::string& level, const std::string& message,
                      const std::string& source) {
    // Add to in-memory buffer
    LogEntry entry;
    entry.timestamp = getCurrentTimestamp();
    entry.level = level;
    entry.message = message;
    entry.source = source;

    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_buffer_.push_back(entry);
        if (log_buffer_.size() > MAX_BUFFER_SIZE) {
            log_buffer_.pop_front();
        }
    }

    // Log to spdlog
    if (impl_->logger) {
        std::string formatted = "[" + source + "] " + message;
        if (level == "DEBUG") impl_->logger->debug(formatted);
        else if (level == "INFO") impl_->logger->info(formatted);
        else if (level == "WARN") impl_->logger->warn(formatted);
        else if (level == "ERROR") impl_->logger->error(formatted);
    }
}

void LogManager::debug(const std::string& msg, const std::string& src) { log("DEBUG", msg, src); }
void LogManager::info(const std::string& msg, const std::string& src) { log("INFO", msg, src); }
void LogManager::warn(const std::string& msg, const std::string& src) { log("WARN", msg, src); }
void LogManager::error(const std::string& msg, const std::string& src) { log("ERROR", msg, src); }

} // namespace yolo_dashboard
