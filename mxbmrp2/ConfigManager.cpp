
// ConfigManager.cpp

#include "pch.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// Definition of defaultValues_
const std::unordered_map<std::string, std::string> ConfigManager::defaultValues_ = {
    // Memory configuration
    {"local_server_name_offset", "0x9CA748"},
    {"local_server_name_size", "64"},
    {"local_server_password_offset", "0x9CA78C"},
    {"local_server_password_size", "32"},
    {"remote_server_ip_offset", "0x57F2D4"},
    {"remote_server_ip_size", "4"},
    {"remote_server_port_offset", "0x57F2C2"},
    {"remote_server_port_size", "2"},
    {"remote_server_password_offset", "0x9B1E04"},
    {"remote_server_password_size", "32"},
    {"remote_server_name_offset", "25"},
    {"remote_server_name_size", "64"},

    // Draw configuration
    {"plugin_banner", "true"},
    {"rider_name", "true"},
    {"category", "false"},
    {"bike_id", "false"},
    {"bike_name", "true"},
    {"setup", "false"},
    {"track_id", "false"},
    {"track_name", "true"},
    {"track_length", "false"},
    {"connection", "true"},
    {"server_name", "true"},
    {"server_password", "true"},
    {"event_type", "true"},
    {"session_state", "false"},
    {"conditions", "false"},
    {"air_temperature", "false"}
};

// Singleton instance
ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

// Constructor
ConfigManager::ConfigManager() = default;

// Destructor
ConfigManager::~ConfigManager() = default;

// Load configuration file
void ConfigManager::loadConfig(const std::string& filePath) {
    std::lock_guard<std::mutex> guard(mutex_);
    config_.clear();

    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        Logger::getInstance().log("Failed to open config file: " + filePath + ", using defaults");
    }
    else {
        Logger::getInstance().log("Loading config file: " + filePath);
        std::string line;
        while (std::getline(configFile, line)) {
            // Remove leading/trailing spaces
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }

            size_t delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) {
                continue; // Skip malformed lines
            }

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            config_[key] = value;
        }
    }

    // Fill missing keys with default values
    for (const auto& [key, defaultValue] : defaultValues_) {
        if (config_.find(key) == config_.end()) {
            config_[key] = defaultValue;
        }
    }
}

// Get a configuration value
std::string ConfigManager::getValue(const std::string& key) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }

    // Log the missing key
    Logger::getInstance().log("'" + key + "' is missing, returning empty string");
    return {};
}
