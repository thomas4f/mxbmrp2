
// ConfigManager.h

#pragma once

#include <variant>
#include <filesystem> 
#include <string>
#include <unordered_map>
#include <type_traits>
#include <mutex>

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

    // Writes all default options to disk, overwriting or creating the file.
    void writeDefaultConfig(const std::filesystem::path& filePath);

    // Loads the configuration file and fills missing keys with default values
    void loadConfig(const std::filesystem::path& filePath);

    // Templated function to retrieve a configuration value
    template<typename T>
    T getValue(const std::string& key) {
        // only these Ts are allowed:
        static_assert(
            std::is_same_v<T, bool> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, unsigned long> ||
            std::is_same_v<T, std::string>,
            "ConfigManager::getValue<T>: T must be bool, float, unsigned long, or std::string"
            );

        std::lock_guard<std::mutex> lk(mutex_);
        auto it = config_.find(key);
        if (it != config_.end()) {
            if (auto val = std::get_if<T>(&it->second)) {
                return *val;
            }
            else {
                Logger::getInstance().log(
                    "Type mismatch for key '" + key + "', using default value."
                );
            }
        }
        else {
            Logger::getInstance().log(
                "Key '" + key + "' not found, using default value."
            );
        }

        // Fallback to default
        auto optIt = configOptions_.find(key);
        if (optIt != configOptions_.end()) {
            try {
                return std::get<T>(optIt->second.defaultValue);
            }
            catch (const std::bad_variant_access&) {
                Logger::getInstance().log(
                    "Default value type mismatch for key '" + key +
                    "'. Returning default-constructed value."
                );
            }
        }

        // Final fallback
        return T{};
    }

private:
    ConfigManager();
    ~ConfigManager();

    // Delete copy constructor and assignment operator
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Configuration data stored as variants
    std::unordered_map<std::string, ConfigValue> config_;

    // Single map containing both type and default value
    static const std::unordered_map<std::string, ConfigOption> configOptions_;

    // Helper functions for type validation
    bool isValidBool(const std::string& value);
    bool isValidFloat(const std::string& value);
    bool isValidUlong(const std::string& value);
    std::mutex mutex_;
};
