
// Plugin.cpp

#include "pch.h"
#include "Plugin.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "MemReader.h"
#include "KeyPressHandler.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

// Constants
constexpr size_t MAX_STRING_LENGTH = 32;
constexpr const char* PLUGIN_VERSION = "mxbmrp2 v0.9.3";
constexpr const char* DATA_DIR = "mxbmrp2_data\\";
constexpr const char* LOG_FILE = "mxbmrp2.log";
constexpr const char* CONFIG_FILE = "mxbmrp2.ini";
static constexpr UINT HOTKEY = 'R';

// Singleton instance
Plugin& Plugin::getInstance() {
    static Plugin instance;
    return instance;
}

// Constructor
Plugin::Plugin()
    : configManager_(ConfigManager::getInstance()), memReader_(MemReader::getInstance()) {
    Logger::getInstance().log("Plugin instance created");
}

// Destructor
Plugin::~Plugin() {
    Logger::getInstance().log("Plugin instance destroyed");
}

// Initialize the plugin and other classes
void Plugin::initialize() {
    // Initialize Logger first
    Logger::getInstance().setLogFileName("plugins\\" + std::string(DATA_DIR) + std::string(LOG_FILE));
    Logger::getInstance().log(std::string(PLUGIN_VERSION) + " initialized.");

    // Initialize ConfigManager
    configManager_.loadConfig("plugins\\" + std::string(DATA_DIR) + std::string(CONFIG_FILE));

    // Initialize MemReader
    memReader_.initialize();

    // Now that ConfigManager is loaded, load Draw configuration
    setDisplayConfig();

    // Initialize KeyPressHandler with the toggleDisplay callback
    keyPressHandler_ = std::make_unique<KeyPressHandler>([this]() { this->toggleDisplay(); }, HOTKEY);
    Logger::getInstance().log("KeyPressHandler initialized with hotkey CTRL + " + std::string(1, static_cast<char>(HOTKEY)));
}

// Shutdown the plugin
void Plugin::shutdown() {
    Logger::getInstance().log("Plugin shutting down");
}

// Load Draw-related config values
void Plugin::setDisplayConfig() {
    try {
        // Load configuration values
        displayConfig_.fontName = std::string(DATA_DIR) + configManager_.getValue("font_name");
        displayConfig_.fontSize = std::stof(configManager_.getValue("font_size"));
        displayConfig_.lineHeight = displayConfig_.fontSize * 1.1f;
        displayConfig_.fontColor = std::stoul(configManager_.getValue("font_color"), nullptr, 16);
        displayConfig_.backgroundColor = std::stoul(configManager_.getValue("background_color"), nullptr, 16);
        displayConfig_.positionX = std::stof(configManager_.getValue("position_x"));
        displayConfig_.positionY = std::stof(configManager_.getValue("position_y"));
        displayConfig_.quadWidth = displayConfig_.lineHeight * MAX_STRING_LENGTH / 3; // Close enough

        // Check if the font file exists in the "/plugins/" directory
        std::filesystem::path fontPath = std::filesystem::path("plugins") / displayConfig_.fontName;
        if (!std::filesystem::exists(fontPath)) {
            throw std::runtime_error("Font file not found: " + fontPath.string());
        }

        Logger::getInstance().log("Draw configuration loaded successfully");
    }
    catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Error loading Draw configuration: ") + e.what());
        Logger::getInstance().log("Falling back to default Draw configuration values");

        // Define default values inline
        displayConfig_.fontSize = 0.02f;
        displayConfig_.fontColor = 0xFFFFFFFF;
        displayConfig_.backgroundColor = 0x7F000000;
        displayConfig_.positionX = 0.0f;
        displayConfig_.positionY = 0.0f;
    }
}

// updateDataKeys
void Plugin::updateDataKeys(const std::unordered_map<std::string, std::string>& dataKeys) {
    Logger::getInstance().log("Updating data keys");

    std::lock_guard<std::mutex> guard(mutex_);

    // Update or add keys from the input map to allDataKeys_
    for (const auto& [key, value] : dataKeys) {
        allDataKeys_[key] = value;
    }

    // Prepare a temporary buffer for data keys to be displayed
    std::vector<std::string> displayKeysBuffer;

    // Inline utility to check if a config key is enabled
    auto isEnabled = [this](const std::string& cfgKey) {
        return configManager_.getValue(cfgKey) == "true";
    };

    // Update keys to display based on configuration and availability
    for (const auto& [configKey, displayName] : configKeyToDisplayNameMap) {
        if (isEnabled(configKey)) {
            const auto& value = allDataKeys_[configKey];
            if (!value.empty()) {

                std::string displayString;

                // Special cases
                if (configKey == "plugin_banner") {
                    displayString = value;
                } else if (configKey == "rider_name" && isEnabled("race_number") && !allDataKeys_["race_number"].empty()) {
                    displayString = displayName + ": " + allDataKeys_["race_number"] + " " + value;
                }
                else { // Everything else
                    displayString = displayName + ": " + value;
                }

                displayKeysBuffer.emplace_back(displayString.substr(0, MAX_STRING_LENGTH));
            }
        }
    }

    // Atomically update the class member with the new display keys
    dataKeysToDisplay_ = std::move(displayKeysBuffer);
}

// Public getter for keys to display
std::vector<std::string> Plugin::getDisplayKeys() {
    std::lock_guard<std::mutex> guard(mutex_);
    return dataKeysToDisplay_;
}

// Helper to convert event types
std::string Plugin::getEventTypeDisplayName(int type, const std::string& connectionType) {
    static const std::unordered_map<int, std::string> eventTypeNames = {
        {1, "Testing"},
        {2, "Race"},
        {4, "Straight Rhythm"}
    };

    // Edge case: Testing when online is called Open Practice
    if (type == 1 && connectionType != "Offline") {
        return "Open Practice";
    }

    // Lookup the type in the map
    auto it = eventTypeNames.find(type);
    return it != eventTypeNames.end() ? it->second : "Unknown";
}

// Helper to convert session names
std::string Plugin::getSessionDisplayName(int type, int session) {
    static const std::unordered_map<int, std::unordered_map<int, std::string>> sessionStateNames = {
        {1, { // Testing
            {0, "Waiting"},
            {1, "In progress"}
        }},
        {2, { // Race
            {0, "Waiting"},
            {1, "Practice"},
            {2, "Pre-Qualify"},
            {3, "Qualify"},
            {4, "?"}, // Not used?
            {5, "Warmup"},
            {6, "Race 1"},
            {7, "Race 2"}
        }},
        {4, { // Straight Rhythm
            {0, "Waiting"},
            {1, "Practice"},
            {2, "Round"},
            {3, "Quarter-Finals"},
            {4, "Semi-Finals"},
            {5, "Final"}
        }}
    };

    auto typeIt = sessionStateNames.find(type);
    if (typeIt != sessionStateNames.end()) {
        auto sessionIt = typeIt->second.find(session);
        if (sessionIt != typeIt->second.end()) {
            return sessionIt->second;
        }
    }
    return "Unknown";
}

// Helper to convert session state names
std::string Plugin::getSessionStateDisplayName(int sessionState) {
    static const std::unordered_map<int, std::string> sessionStateNames = {
        {0, ""},
        {16, "In Progress"},
        {32, "Completed"},
        {64, "Sighting Lap"},
        {256, "Pre-Start"},
        {512, "Race Over"},
        {1024, "Completed"}
    };

    // Find the session state in the map
    auto it = sessionStateNames.find(sessionState);
    if (it != sessionStateNames.end()) {
        return it->second;
    }
    return "Unknown";
}

// Helper to convert conditions to a friendly name
std::string Plugin::getConditionsDisplayName(int condition) {
    static const std::unordered_map<int, std::string> conditionsMap = {
        {0, "Clear"},
        {1, "Cloudy"},
        {2, "Rainy"}
    };

    auto it = conditionsMap.find(condition);
    return it != conditionsMap.end() ? it->second : "Unknown";
}

// Helper to convert IP or port to a human-readable format
std::string convertRawData(const std::string& rawData) {
    if (rawData.size() == 4) {
        std::ostringstream oss;
        for (size_t i = 0; i < 4; ++i) {
            oss << (i ? "." : "") << static_cast<int>(static_cast<unsigned char>(rawData[i]));
        }
        return oss.str();
    }
    if (rawData.size() == 2) {
        uint16_t port;
        memcpy(&port, rawData.data(), sizeof(port));
        return std::to_string(ntohs(port));
    }
    return "";
}

// Helper to get custom data
std::string Plugin::getCustomData(const std::string& keyOffset, const std::string& keySize, const std::string& label) {
    // Fetch offsets and sizes
    uintptr_t offset = std::stoul(configManager_.getValue(keyOffset), nullptr, 16);
    size_t size = std::stoul(configManager_.getValue(keySize));

    // Read the memory at the given offset and size
    std::string rawValue = memReader_.readStringAtOffset(offset, size, label);

    if (label == "local_server_name" || label == "local_server_password" || label == "remote_server_password") {
        // Truncate at the first null byte
        return std::string(rawValue.c_str(), strnlen(rawValue.c_str(), size));
    }
    else if (label == "remote_server_ip" || label == "remote_server_port") {
        // Store raw data in temporary variables
        if (label == "remote_server_ip") {
            rawRemoteServer_.insert(rawRemoteServer_.end(), rawValue.begin(), rawValue.end());
        }
        else if (label == "remote_server_port") {
            rawRemoteServer_.insert(rawRemoteServer_.end(), rawValue.begin(), rawValue.end());
        }

        // Convert to a human-readable format
        return convertRawData(rawValue);
    }

    return "";
}

// Maps config keys to display names and sets display order
const std::vector<std::pair<std::string, std::string>> Plugin::configKeyToDisplayNameMap = {
    {"plugin_banner", "Plugin Banner"},
    {"rider_name", "Rider Name"},
    {"category", "Category"},
    {"bike_id", "Bike ID"},
    {"bike_name", "Bike Name"},
    {"setup", "Setup"},
    {"track_id", "Track ID"},
    {"track_name", "Track Name"},
    {"track_length", "Track Length"},
    {"connection", "Connection"},
    {"server_name", "Server Name"},
    {"server_password", "Server Password"},
    {"event_type", "Event Type"},
    {"session", "Session"},
    {"session_state", "Session State"},
    {"conditions", "Conditions"},
    {"air_temperature", "Air Temperature"}
};

// EventInit
void Plugin::onEventInit(const SPluginsBikeEvent_t& eventData) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    type_ = eventData.m_iType;
    // we cant rely on type alone since it may be 1 (Testing) for Open Practice
    localServerName_ = getCustomData("local_server_name_offset", "local_server_name_size", "local_server_name");
    remoteServerIP_ = getCustomData("remote_server_ip_offset", "remote_server_ip_size", "remote_server_ip");

    if (!localServerName_.empty()) { // Host
        serverName_ = localServerName_;
        connectionType_ = "Host";
        serverPassword_ = getCustomData("local_server_password_offset", "local_server_password_size", "local_server_password");
    }
    else if (remoteServerIP_ != "0.0.0.0") { // Client
        remoteServerPort_ = getCustomData("remote_server_port_offset", "remote_server_port_size", "remote_server_port");
        serverPassword_ = getCustomData("remote_server_password_offset", "remote_server_password_size", "remote_server_password");
        connectionType_ = "Client";

        // Search for server name
        std::string searchPattern(reinterpret_cast<char*>(rawRemoteServer_.data()), rawRemoteServer_.size());

        // Get remote server name size and offset from configuration
        size_t remoteServerNameSize = std::stoul(configManager_.getValue("remote_server_name_size"));
        uintptr_t remoteServerNameOffset = std::stoul(configManager_.getValue("remote_server_name_offset"), nullptr, 16);

        serverName_ = memReader_.searchMemory(searchPattern, remoteServerNameSize, remoteServerNameOffset);

    } else {
        connectionType_ = "Offline";
    }

    std::unordered_map<std::string, std::string> dataKeys = {
        {"plugin_banner", PLUGIN_VERSION},
        {"rider_name", eventData.m_szRiderName},
        {"category", eventData.m_szCategory},
        {"bike_id", eventData.m_szBikeID},
        {"bike_name", eventData.m_szBikeName},
        {"track_id", eventData.m_szTrackID},
        {"track_name", eventData.m_szTrackName},
        {"track_length", std::to_string(static_cast<int>(std::round(eventData.m_fTrackLength))) + " m"},
        {"server_name", serverName_},
        {"server_password", serverPassword_},
        {"connection", connectionType_},
        {"event_type", getEventTypeDisplayName(eventData.m_iType, connectionType_)}
    };

    updateDataKeys(dataKeys);
}

// RunInit
void Plugin::onRunInit(const SPluginsBikeSession_t& sessionData) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    std::unordered_map<std::string, std::string> dataKeys = {
        {"setup", std::strlen(sessionData.m_szSetupFileName) > 0 ? std::string(sessionData.m_szSetupFileName).substr(1) : "Default"},
        {"session", getSessionDisplayName(type_, sessionData.m_iSession)},
        {"conditions", getConditionsDisplayName(sessionData.m_iConditions)},
        {"air_temperature", std::to_string(static_cast<int>(std::round(sessionData.m_fAirTemperature))) + "°C"}
    };

    updateDataKeys(dataKeys);
}

// RaceSession
void Plugin::onRaceSession(const SPluginsRaceSession_t& raceSession) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    updateDataKeys({ {"session_state", getSessionStateDisplayName(raceSession.m_iSessionState)} });
}

// RaceSessionState
void Plugin::onRaceSessionState(const SPluginsRaceSessionState_t& raceSessionState) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    updateDataKeys({ {"session_state", getSessionStateDisplayName(raceSessionState.m_iSessionState)} });
}

// RaceAddEntry
void Plugin::onRaceAddEntry(const SPluginsRaceAddEntry_t& raceAddEntry) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    // Check whether the entry is in fact the local player
    if (raceAddEntry.m_szName == allDataKeys_["rider_name"] && raceAddEntry.m_szBikeName == allDataKeys_["bike_name"]) {
        updateDataKeys({ {"race_number", std::to_string(raceAddEntry.m_iRaceNum) } });
    }
}

// EventDeinit
void Plugin::onEventDeinit() {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    // clear custom data keys
    localServerName_.clear();
    remoteServerIP_.clear();
    remoteServerPort_.clear();
    rawRemoteServer_.clear();
    serverName_.clear();
    serverPassword_.clear();
    connectionType_.clear();

    // Clear the display keys as well
    allDataKeys_.clear();
    dataKeysToDisplay_.clear();
}

// Define the callback function
void Plugin::toggleDisplay() {

    // Toggle display
    displayEnabled_ = !displayEnabled_;
        if (!displayEnabled_) {
            dataKeysToDisplay_.clear();
            Logger::getInstance().log("Display disabled.");
        } else {
            configManager_.loadConfig("plugins\\" + std::string(DATA_DIR) + std::string(CONFIG_FILE));
            setDisplayConfig();

            // Re-populate displayKeysToDisplay_ based on current allDataKeys_
            updateDataKeys(allDataKeys_);
            Logger::getInstance().log("Display enabled.");
        }
}
