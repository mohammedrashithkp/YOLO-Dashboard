#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <memory>

namespace yolo_dashboard {

struct LogEntry {
    std::string timestamp;
    std::string level;    // "DEBUG", "INFO", "WARN", "ERROR"
    std::string message;
    std::string source;   // module name
};

/// Manages structured logging with spdlog backend.
/// Provides log retrieval, level control, and file download.
class LogManager {
public:
    LogManager();
    ~LogManager();

    /// Initialize logging with file output.
    void initialize(const std::string& log_file, const std::string& level = "info",
                    int max_size_mb = 50, int max_files = 5);

    /// Get recent log entries (thread-safe in-memory buffer).
    std::vector<LogEntry> getRecentLogs(int count = 100,
                                         const std::string& level_filter = "") const;

    /// Set log level at runtime.
    void setLevel(const std::string& level);

    /// Get current log level.
    std::string getLevel() const;

    /// Get the path to the current log file.
    std::string getLogFilePath() const { return log_file_path_; }

    /// Log a message (also added to in-memory buffer).
    void log(const std::string& level, const std::string& message,
             const std::string& source = "app");

    // Convenience methods
    void debug(const std::string& msg, const std::string& src = "app");
    void info(const std::string& msg, const std::string& src = "app");
    void warn(const std::string& msg, const std::string& src = "app");
    void error(const std::string& msg, const std::string& src = "app");

private:
    mutable std::mutex log_mutex_;
    std::deque<LogEntry> log_buffer_;
    std::string log_file_path_;
    std::string current_level_ = "info";

    static constexpr int MAX_BUFFER_SIZE = 5000;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace yolo_dashboard
