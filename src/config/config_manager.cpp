#include "yolo_dashboard/config/config_manager.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>

namespace yolo_dashboard {

ConfigManager::ConfigManager() {
    resetToDefaults();
}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    try {
        YAML::Node root = YAML::LoadFile(filepath);
        return loadFromNode(root, filepath);
    } catch (const YAML::Exception& e) {
        std::cerr << "[Config] Failed to load " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::loadFromString(const std::string& yaml_content) {
    try {
        YAML::Node root = YAML::Load(yaml_content);
        return loadFromNode(root, "");
    } catch (const YAML::Exception& e) {
        std::cerr << "[Config] Failed to parse YAML: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::loadFromNode(const YAML::Node& root, const std::string& filepath) {
    try {
        // Server
        if (root["server"]) {
            if (root["server"]["port"]) config_.server_port = root["server"]["port"].as<int>();
            if (root["server"]["host"]) config_.server_host = root["server"]["host"].as<std::string>();
        }

        // Camera
        if (root["camera"]) {
            if (root["camera"]["device_id"]) config_.camera_device_id = root["camera"]["device_id"].as<int>();
            if (root["camera"]["resolution"] && root["camera"]["resolution"].IsSequence() &&
                root["camera"]["resolution"].size() >= 2) {
                config_.camera_resolution.width = root["camera"]["resolution"][0].as<int>();
                config_.camera_resolution.height = root["camera"]["resolution"][1].as<int>();
            }
            if (root["camera"]["fps"]) config_.camera_fps = root["camera"]["fps"].as<int>();
        }

        // Inference
        if (root["inference"]) {
            if (root["inference"]["mode"]) config_.inference_mode = root["inference"]["mode"].as<std::string>();
            if (root["inference"]["model_path"]) config_.model_path = root["inference"]["model_path"].as<std::string>();
            if (root["inference"]["confidence_threshold"]) config_.confidence_threshold = root["inference"]["confidence_threshold"].as<float>();
            if (root["inference"]["nms_threshold"]) config_.nms_threshold = root["inference"]["nms_threshold"].as<float>();
            if (root["inference"]["cpu_threads"]) config_.cpu_threads = root["inference"]["cpu_threads"].as<int>();
        }

        // Recording
        if (root["recording"]) {
            if (root["recording"]["enabled"]) config_.recording_enabled = root["recording"]["enabled"].as<bool>();
            if (root["recording"]["output_dir"]) config_.recording_output_dir = root["recording"]["output_dir"].as<std::string>();
            if (root["recording"]["codec"]) config_.recording_codec = root["recording"]["codec"].as<std::string>();
            if (root["recording"]["annotated"]) config_.recording_annotated = root["recording"]["annotated"].as<bool>();
        }

        // Logging
        if (root["logging"]) {
            if (root["logging"]["level"]) config_.log_level = root["logging"]["level"].as<std::string>();
            if (root["logging"]["file"]) config_.log_file = root["logging"]["file"].as<std::string>();
            if (root["logging"]["max_size_mb"]) config_.log_max_size_mb = root["logging"]["max_size_mb"].as<int>();
            if (root["logging"]["max_files"]) config_.log_max_files = root["logging"]["max_files"].as<int>();
        }

        // Metrics
        if (root["metrics"]) {
            if (root["metrics"]["export_interval_sec"]) config_.metrics_export_interval_sec = root["metrics"]["export_interval_sec"].as<int>();
            if (root["metrics"]["output_dir"]) config_.metrics_output_dir = root["metrics"]["output_dir"].as<std::string>();
            if (root["metrics"]["format"]) config_.metrics_format = root["metrics"]["format"].as<std::string>();
        }

        // Auth
        if (root["auth"]) {
            if (root["auth"]["username"]) config_.auth_username = root["auth"]["username"].as<std::string>();
            if (root["auth"]["password_hash"]) config_.auth_password_hash = root["auth"]["password_hash"].as<std::string>();
            if (root["auth"]["token_ttl_hours"]) config_.auth_token_ttl_hours = root["auth"]["token_ttl_hours"].as<int>();
        }

        current_filepath_ = filepath;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error parsing config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::saveToFile(const std::string& filepath) const {
    try {
        std::ofstream fout(filepath);
        if (!fout.is_open()) {
            std::cerr << "[Config] Cannot open file for writing: " << filepath << std::endl;
            return false;
        }
        fout << toYamlString();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Config] Failed to save: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::toYamlString() const {
    YAML::Node root;

    // Server
    root["server"]["port"] = config_.server_port;
    root["server"]["host"] = config_.server_host;

    // Camera
    root["camera"]["device_id"] = config_.camera_device_id;
    root["camera"]["resolution"].push_back(config_.camera_resolution.width);
    root["camera"]["resolution"].push_back(config_.camera_resolution.height);
    root["camera"]["fps"] = config_.camera_fps;

    // Inference
    root["inference"]["mode"] = config_.inference_mode;
    root["inference"]["model_path"] = config_.model_path;
    root["inference"]["confidence_threshold"] = config_.confidence_threshold;
    root["inference"]["nms_threshold"] = config_.nms_threshold;
    root["inference"]["cpu_threads"] = config_.cpu_threads;

    // Recording
    root["recording"]["enabled"] = config_.recording_enabled;
    root["recording"]["output_dir"] = config_.recording_output_dir;
    root["recording"]["codec"] = config_.recording_codec;
    root["recording"]["annotated"] = config_.recording_annotated;

    // Logging
    root["logging"]["level"] = config_.log_level;
    root["logging"]["file"] = config_.log_file;
    root["logging"]["max_size_mb"] = config_.log_max_size_mb;
    root["logging"]["max_files"] = config_.log_max_files;

    // Metrics
    root["metrics"]["export_interval_sec"] = config_.metrics_export_interval_sec;
    root["metrics"]["output_dir"] = config_.metrics_output_dir;
    root["metrics"]["format"] = config_.metrics_format;

    // Auth
    root["auth"]["username"] = config_.auth_username;
    root["auth"]["password_hash"] = config_.auth_password_hash;
    root["auth"]["token_ttl_hours"] = config_.auth_token_ttl_hours;

    YAML::Emitter emitter;
    emitter << root;
    return emitter.c_str();
}

bool ConfigManager::updateFromString(const std::string& yaml_content) {
    // Parse and merge with existing config
    return loadFromString(yaml_content);
}

void ConfigManager::resetToDefaults() {
    config_ = AppConfig{};
}

std::string ConfigManager::validate() const {
    if (config_.server_port < 1 || config_.server_port > 65535) {
        return "Server port must be between 1 and 65535";
    }
    if (config_.camera_resolution.width < 1 || config_.camera_resolution.height < 1) {
        return "Camera resolution must be positive";
    }
    if (config_.confidence_threshold < 0.0f || config_.confidence_threshold > 1.0f) {
        return "Confidence threshold must be between 0 and 1";
    }
    if (config_.nms_threshold < 0.0f || config_.nms_threshold > 1.0f) {
        return "NMS threshold must be between 0 and 1";
    }
    if (config_.cpu_threads < 1) {
        return "CPU threads must be at least 1";
    }
    return ""; // Valid
}

} // namespace yolo_dashboard
