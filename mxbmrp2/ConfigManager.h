
// ConfigManager.h

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

class ConfigManager {
public:
    // Retrieves the singleton instance of ConfigManager
    static ConfigManager& getInstance();

    // Loads the configuration file and fills missing keys with default values
    void loadConfig(const std::string& filePath);

    // Retrieves a configuration value, falling back to default if missing
    std::string getValue(const std::string& key);

private:
    ConfigManager();
    ~ConfigManager();

    // Delete copy constructor and assignment operator
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Mutex for thread-safe operations
    std::mutex mutex_;

    // Configuration data
    std::unordered_map<std::string, std::string> config_;

    // Default values
    static const std::unordered_map<std::string, std::string> defaultValues_;
};
