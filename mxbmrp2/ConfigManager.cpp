
// ConfigManager.cpp

#include "pch.h"
#include "Constants.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

// Definition of configOptions_
const std::unordered_map<std::string, ConfigManager::ConfigOption> ConfigManager::configOptions_ = {
    // Draw configuration
    {"plugin_banner", {ConfigType::BOOL, true}},
    {"race_number", {ConfigType::BOOL, true}},
    {"rider_name", {ConfigType::BOOL, true}},
    {"bike_category", {ConfigType::BOOL, false}},
    {"bike_id", {ConfigType::BOOL, false}},
    {"bike_name", {ConfigType::BOOL, true}},
    {"setup_name", {ConfigType::BOOL, false}},
    {"track_id", {ConfigType::BOOL, false}},
    {"track_name", {ConfigType::BOOL, true}},
    {"track_length", {ConfigType::BOOL, false}},
    {"connection_type", {ConfigType::BOOL, false}},
    {"server_name", {ConfigType::BOOL, true}},
    {"server_password", {ConfigType::BOOL, false}},
    {"server_location", {ConfigType::BOOL, false}},
    {"server_ping", {ConfigType::BOOL, true}},
    {"event_type", {ConfigType::BOOL, false}},
    {"session_type", {ConfigType::BOOL, false}},
    {"session_state", {ConfigType::BOOL, false}},
    {"conditions", {ConfigType::BOOL, false}},
    {"air_temperature", {ConfigType::BOOL, false}},

    // GUI configuration
    {"default_enabled", {ConfigType::BOOL, true}},
    {"position_x", {ConfigType::FLOAT, 0.0f}},
    {"position_y", {ConfigType::FLOAT, 0.0f}},
    {"font_name", {ConfigType::STRING, std::string("CQ Mono.fnt")}},
    {"font_size", {ConfigType::FLOAT, 0.025f}},
    {"font_color", {ConfigType::ULONG, 0xFFFFFFFFUL}},
    {"background_color", {ConfigType::ULONG, 0x7F000000UL}},

    // Memory configuration
    {"local_server_name_offset", {ConfigType::ULONG, 0x9CA748UL}},
    {"local_server_name_size", {ConfigType::ULONG, 64UL}},
    {"local_server_password_offset", {ConfigType::ULONG, 0x9CA78CUL}},
    {"local_server_password_size", {ConfigType::ULONG, 32UL}},
    {"local_server_location_offset", {ConfigType::ULONG, 0x9CA7ACUL}},
    {"local_server_location_size", {ConfigType::ULONG, 32UL}},
    {"remote_server_ip_offset", {ConfigType::ULONG, 0x57F2D4UL}},
    {"remote_server_ip_size", {ConfigType::ULONG, 4UL}},
    {"remote_server_port_offset", {ConfigType::ULONG, 0x57F2C2UL}},
    {"remote_server_port_size", {ConfigType::ULONG, 2UL}},
    {"remote_server_password_offset", {ConfigType::ULONG, 0x9B1E04UL}},
    {"remote_server_password_size", {ConfigType::ULONG, 32UL}},
    {"remote_server_name_offset", {ConfigType::ULONG, 0x19UL}},
    {"remote_server_name_size", {ConfigType::ULONG, 64UL}},
    {"remote_server_location_offset", {ConfigType::ULONG, 0x73UL}},
    {"remote_server_location_size", {ConfigType::ULONG, 32UL}},
    {"remote_server_ping_offset", {ConfigType::ULONG, 0xFEUL}},
    {"remote_server_ping_size", {ConfigType::ULONG, 2UL}}
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

// Helper function to validate boolean
bool ConfigManager::isValidBool(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower == "true" || lower == "false";
}

// Helper function to validate float
bool ConfigManager::isValidFloat(const std::string& value) {
    try {
        size_t pos;
        (void)std::stof(value, &pos);
        return pos == value.length();
    }
    catch (...) {
        return false;
    }
}

// Helper function to validate unsigned long
bool ConfigManager::isValidUlong(const std::string& value) {
    try {
        size_t pos;
        (void)std::stoul(value, &pos, 0);
        return pos == value.length();
    }
    catch (...) {
        return false;
    }
}

// Load configuration file with type validation
void ConfigManager::loadConfig(const std::string& filePath) {
    std::lock_guard<std::mutex> guard(mutex_);
    config_.clear();

    std::ifstream configFile(filePath);
    if (configFile.is_open()) {
        Logger::getInstance().log("Loading config file: " + filePath);
        std::string line;
        int lineNumber = 0;
        while (std::getline(configFile, line)) {
            lineNumber++;

            // Remove leading/trailing spaces
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Skip malformed lines
            size_t delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) {
                Logger::getInstance().log("Malformed line " + std::to_string(lineNumber) + ": " + line);
                continue;
            }

            // Split the configuration line into key and value by the '=' delimiter
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Check if key is known
            auto optionIt = configOptions_.find(key);
            if (optionIt != configOptions_.end()) {
                const ConfigOption& option = optionIt->second;
                try {
                    switch (option.type) {
                    case ConfigType::BOOL: {
                        std::string lower = value;
                        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                        if (lower == "true") {
                            config_[key] = true;
                        }
                        else if (lower == "false") {
                            config_[key] = false;
                        }
                        else {
                            throw std::invalid_argument("Invalid BOOL value");
                        }
                        break;
                    }
                    case ConfigType::FLOAT: {
                        if (!isValidFloat(value)) {
                            throw std::invalid_argument("Invalid FLOAT value");
                        }
                        float floatValue = std::stof(value);
                        config_[key] = floatValue;
                        break;
                    }
                    case ConfigType::ULONG: {
                        if (!isValidUlong(value)) {
                            throw std::invalid_argument("Invalid ULONG value");
                        }
                        unsigned long ulongValue = std::stoul(value, nullptr, 0);
                        config_[key] = ulongValue;
                        break;
                    }
                    case ConfigType::STRING: {
                        config_[key] = value;

                        // If the key is "font_name", perform file existence check
                        if (key == "font_name") {
                            std::filesystem::path fontPath = std::filesystem::path("plugins") / DATA_DIR / value;
                            if (!std::filesystem::exists(fontPath) || !std::filesystem::is_regular_file(fontPath)) {
                                throw std::runtime_error("Font file not found: " + fontPath.string());
                            }
                        }

                        break;
                    }
                    default:
                        throw std::invalid_argument("Unknown ConfigType");
                    }
                }
                catch (const std::exception& e) {
                    // On failure, use default value
                    config_[key] = option.defaultValue;
                    Logger::getInstance().log("Error parsing key '" + key + "' on line " + std::to_string(lineNumber) + ": " + e.what() + ". Using default value.");
                }
            }
            else {
                // Unknown key, ignore or log
                Logger::getInstance().log("Unknown configuration key on line " + std::to_string(lineNumber) + ": " + key);
            }
        }
    }
    else {
        Logger::getInstance().log("Failed to open config file: " + filePath);
    }

    // Fill missing keys with default values
    for (const auto& [key, option] : configOptions_) {
        if (config_.find(key) == config_.end()) {
            config_[key] = option.defaultValue;
            Logger::getInstance().log("Missing configuration key '" + key + "'. Using default value.");
        }
    }
}
