
// ConfigManager.cpp

#include "pch.h"

#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <iomanip>

#include "Constants.h"
#include "ConfigManager.h"
#include "Logger.h"

// Definition of configOptions_
const std::unordered_map<std::string, ConfigManager::ConfigOption> ConfigManager::configOptions_ = {
    // Draw configuration
    {"plugin_banner", {ConfigType::BOOL, true}},
    {"race_number", {ConfigType::BOOL, true}},
    {"rider_name", {ConfigType::BOOL, true}},
    {"bike_category", {ConfigType::BOOL, false}},
    {"bike_id", {ConfigType::BOOL, false}},
    {"bike_name", {ConfigType::BOOL, true}},
    {"setup_name", {ConfigType::BOOL, true}},
    {"remaining_tearoffs", {ConfigType::BOOL, false}},
    {"track_id", {ConfigType::BOOL, false}},
    {"track_name", {ConfigType::BOOL, true}},
    {"track_length", {ConfigType::BOOL, false}},
    {"connection_type", {ConfigType::BOOL, false}},
    {"server_name", {ConfigType::BOOL, true}},
    {"server_password", {ConfigType::BOOL, false}},
    {"server_location", {ConfigType::BOOL, false}},
    {"server_ping", {ConfigType::BOOL, true}},
    {"server_clients", {ConfigType::BOOL, true}},
    {"event_type", {ConfigType::BOOL, false}},
    {"session_type", {ConfigType::BOOL, false}},
    {"session_state", {ConfigType::BOOL, false}},
    {"session_duration", {ConfigType::BOOL, true}},
    {"conditions", {ConfigType::BOOL, false}},
    {"air_temperature", {ConfigType::BOOL, false}},
    {"track_deformation",{ConfigType::BOOL, false}},
    {"combo_time", {ConfigType::BOOL, false}},
    {"total_time", {ConfigType::BOOL, true}},
    {"discord_status", { ConfigType::BOOL, false }},

    // GUI configuration
    {"default_enabled", {ConfigType::BOOL, true}},
    {"position_x", {ConfigType::FLOAT, 0.0f}},
    {"position_y", {ConfigType::FLOAT, 0.0f}},

    {"font_name", {ConfigType::STRING, std::string("CQ Mono.fnt")}},
    {"font_size", {ConfigType::FLOAT, 0.025f}},
    {"font_color", {ConfigType::ULONG, 0xFFFFFFFFUL}},
    {"background_color", {ConfigType::ULONG, 0x7F000000UL}},

    // Discord integration
    { "enable_discord_rich_presence", {ConfigType::BOOL, false }},

    // HTML/JSON export
    { "enable_html_export", {ConfigType::BOOL, false }},
    { "enable_json_export", {ConfigType::BOOL, false }},

    // Memory configuration
    {"local_server_name_offset",{ConfigType::ULONG,0x9D6768UL}},
    {"local_server_password_offset",{ConfigType::ULONG,0x9D67ACUL}},
    {"local_server_location_offset",{ConfigType::ULONG,0x9D67CCUL}},
    {"local_server_clients_max_offset",{ConfigType::ULONG,0x9D6820UL}},

    {"remote_server_sockaddr_offset",{ConfigType::ULONG,0x58B2BCUL}},
    {"remote_server_password_offset",{ConfigType::ULONG,0x9BDE04UL}},
    {"remote_server_name_offset",{ConfigType::ULONG,0x1BUL}},
    {"remote_server_location_offset",{ConfigType::ULONG,0x75UL}},
    {"remote_server_ping_offset",{ConfigType::ULONG,0x58B534UL}},
    {"remote_server_clients_max_offset",{ConfigType::ULONG,0x5DUL}},

    {"local_server_remaining_tearoffs_offset",{ConfigType::ULONG,0x9D78BCUL}},
    {"remote_server_remaining_tearoffs_offset",{ConfigType::ULONG,0x108BE0CUL}},

    {"track_deformation_offset",{ConfigType::ULONG,0x58B708UL}},

    {"server_categories_offset",{ConfigType::ULONG,0x58B634UL}},
    {"server_track_id_offset",{ConfigType::ULONG,0x58B5D4UL}},
    {"server_clients_offset",{ConfigType::ULONG,0xE49F28UL}},

    {"connection_string_offset",{ConfigType::ULONG,0x559DC0UL}},
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

static std::string stringifyDefault(const ConfigManager::ConfigOption& opt) {
    return std::visit(
        [&](auto&& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            }
            else if constexpr (std::is_same_v<T, unsigned long>) {
                std::ostringstream oss;
                oss << "0x"
                    << std::uppercase << std::hex
                    << v;
                return oss.str();
            }
            else if constexpr (std::is_same_v<T, float>) {
                return std::to_string(v);
            }
            else { // std::string
                return v;
            }
        },
        opt.defaultValue
    );
}

// Write default configuration to file
void ConfigManager::writeDefaultConfig(const std::filesystem::path& outPath) {
    // copy our literal into a mutable std::string
    std::string rendered = DEFAULT_INI_TEMPLATE;

    // replace every {{key}} with its default
    for (auto& [key, opt] : configOptions_) {
        std::string tag = "{{" + key + "}}";
        std::string val = stringifyDefault(opt);

        size_t pos = 0;
        while ((pos = rendered.find(tag, pos)) != std::string::npos) {
            rendered.replace(pos, tag.size(), val);
            pos += val.size();
        }
    }

    // atomically write the file
    auto tmp = outPath.string() + ".tmp";
    std::ofstream ofs(tmp, std::ios::trunc);
    if (!ofs) throw std::runtime_error("Unable to open " + tmp);
    ofs << rendered;
    ofs.close();
    std::filesystem::rename(tmp, outPath);
}

void ConfigManager::loadConfig(const std::filesystem::path& cfgPath) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (!std::filesystem::exists(cfgPath)) {
        Logger::getInstance().log("Config not found, creating defaults: " + cfgPath.string());
        writeDefaultConfig(cfgPath);
    }

    config_.clear();

    std::ifstream configFile(cfgPath);
    if (configFile.is_open()) {
        Logger::getInstance().log("Loading config file: " + cfgPath.string());
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
                                Logger::getInstance().log("Font file missing: " + fontPath.string() + ". Text rendering not work properly.");
                                config_[key] = value;
                                continue;
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
        Logger::getInstance().log("Failed to open config file: " + cfgPath.string());
    }

    // Fill missing keys with default values
    for (const auto& [key, option] : configOptions_) {
        if (config_.find(key) == config_.end()) {
            config_[key] = option.defaultValue;
            Logger::getInstance().log("Missing configuration key '" + key + "'. Using default value.");
        }
    }
}
