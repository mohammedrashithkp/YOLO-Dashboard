#pragma once

#include "yolo_dashboard/common.h"
#include <string>
#include <yaml-cpp/yaml.h>

namespace yolo_dashboard {

/// Manages application configuration using YAML files.
/// Supports load, save, validate, and live update of settings.
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;

    /// Load configuration from a YAML file.
    bool loadFromFile(const std::string& filepath);

    /// Save current configuration to a YAML file.
    bool saveToFile(const std::string& filepath) const;

    /// Load configuration from a YAML string.
    bool loadFromString(const std::string& yaml_content);

    /// Serialize current configuration to a YAML string.
    std::string toYamlString() const;

    /// Get the current configuration (read-only).
    const AppConfig& getConfig() const { return config_; }

    /// Get mutable reference for updating configuration.
    AppConfig& getMutableConfig() { return config_; }

    /// Update configuration from a partial YAML string.
    bool updateFromString(const std::string& yaml_content);

    /// Reset to default configuration.
    void resetToDefaults();

    /// Validate current configuration.
    /// Returns empty string if valid, error description otherwise.
    std::string validate() const;

    /// Get the path of the currently loaded config file.
    std::string getCurrentFilePath() const { return current_filepath_; }

private:
    bool loadFromNode(const YAML::Node& root, const std::string& filepath);
    AppConfig config_;
    std::string current_filepath_;
};

} // namespace yolo_dashboard
