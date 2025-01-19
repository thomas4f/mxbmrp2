
// ConfigManager.h

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <variant>
#include "Logger.h"

class ConfigManager {
public:
    // Supported configuration types
    enum class ConfigType {
        STRING,
        FLOAT,
        ULONG,
        BOOL
    };

    // Define a variant type for configuration values
    using ConfigValue = std::variant<std::string, float, unsigned long, bool>;

    // Struct to hold configuration type and default value
    struct ConfigOption {
        ConfigType type;
        ConfigValue defaultValue;
    };

    // Retrieves the singleton instance of ConfigManager
    static ConfigManager& getInstance();

    // Loads the configuration file and fills missing keys with default values
    void loadConfig(const std::string& filePath);

    // Templated function to retrieve a configuration value
    template<typename T>
    T getValue(const std::string& key) {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = config_.find(key);
        if (it != config_.end()) {
            if (auto val = std::get_if<T>(&(it->second))) {
                return *val;
            }
            else {
                Logger::getInstance().log("Type mismatch for key '" + key + "', using default value.");
            }
        }
        else {
            Logger::getInstance().log("Key '" + key + "' not found, using default value.");
        }

        // Return default value if key is missing or type mismatched
        auto optionIt = configOptions_.find(key);
        if (optionIt != configOptions_.end()) {
            try {
                return std::get<T>(optionIt->second.defaultValue);
            }
            catch (const std::bad_variant_access&) {
                Logger::getInstance().log("Default value type mismatch for key '" + key + "'. Returning default-constructed value.");
            }
        }

        // If no default is found, return a default-constructed value
        return T{};
    }

private:
    ConfigManager();
    ~ConfigManager();

    // Delete copy constructor and assignment operator
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Mutex for thread-safe operations
    std::mutex mutex_;

    // Configuration data stored as variants
    std::unordered_map<std::string, ConfigValue> config_;

    // Single map containing both type and default value
    static const std::unordered_map<std::string, ConfigOption> configOptions_;

    // Helper functions for type validation
    bool isValidBool(const std::string& value);
    bool isValidFloat(const std::string& value);
    bool isValidUlong(const std::string& value);
};
