// Plugin.cpp

#include "pch.h"
#include "Plugin.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "MemReader.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

// Constants
constexpr const char* PLUGIN_VERSION = "mxbmrp2 v0.9";
constexpr const char* DATA_DIR = "mxbmrp2_data\\";

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
    Logger::getInstance().setLogFileName("plugins\\" + std::string(DATA_DIR) + "mxbmrp2.log");
    Logger::getInstance().log(std::string(PLUGIN_VERSION) + " initialized.");

    // Initialize ConfigManager
    configManager_.loadConfig("plugins\\" + std::string(DATA_DIR) + "mxbmrp2.ini");

    // Initialize MemReader
    memReader_.initialize();

    // Now that ConfigManager is loaded, load Draw configuration
    loadDrawConfig();
}


// Shutdown the plugin
void Plugin::shutdown() {
    Logger::getInstance().log("Plugin shutting down");
}

// Method to load Draw-related config values
void Plugin::loadDrawConfig() {
    try {
        // Load configuration values
        drawConfig_.fontName = std::string(DATA_DIR) + configManager_.getValue("font_name");
        drawConfig_.fontSize = std::stof(configManager_.getValue("font_size"));
        drawConfig_.fontColor = std::stoul(configManager_.getValue("font_color"), nullptr, 16);
        drawConfig_.backgroundColor = std::stoul(configManager_.getValue("background_color"), nullptr, 16);
        drawConfig_.positionX = std::stof(configManager_.getValue("position_x"));
        drawConfig_.positionY = std::stof(configManager_.getValue("position_y"));

        // Check if the font file exists in the "/plugins/" directory
        if (!std::filesystem::exists("plugins\\" + drawConfig_.fontName)) {
            throw std::runtime_error("Font file not found: " + drawConfig_.fontName);
        }

        Logger::getInstance().log("Draw configuration loaded successfully");
    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Error loading Draw configuration: ") + e.what());
        Logger::getInstance().log("Falling back to default Draw configuration values");

        // Define default values inline
        drawConfig_.fontSize = 0.02f;
        drawConfig_.fontColor = 0xFFFFFFFF;
        drawConfig_.backgroundColor = 0x7F000000;
        drawConfig_.positionX = 0.0f;
        drawConfig_.positionY = 0.0f;
    }
}

// Getter for display strings for Draw
std::vector<std::string> Plugin::getDisplayStrings() {
    std::lock_guard<std::mutex> guard(displayMutex_);
    return displayStrings_;
}

// Helper to convert event types
std::string getFriendlyEventTypeName(int type) {
    static const std::unordered_map<int, std::string> eventTypeNames = {
        {1, "Testing"},
        {2, "Race"},
        {4, "Straight Rhythm"},
        {-1, "Replay"}
    };

    auto it = eventTypeNames.find(type);
    return it != eventTypeNames.end() ? it->second : "Unknown";
}

// Helper to convert session states
std::string getFriendlySessionStateName(int type, int session) {
    static const std::unordered_map<int, std::unordered_map<int, std::string>> sessionStateNames = {
        {1, { // Testing
            {0, "Waiting"},
            {1, "In Progress"},
            {16, "In Progress (Extended)"},
            {32, "Completed"}
        }},
        {2, { // Race
            {0, "Waiting"},
            {1, "Practice"},
            {2, "Pre-Qualify"},
            {3, "Qualify"},
            {4, "Warmup"},
            {5, "Race 1"},
            {6, "Race 2"},
            {16, "In Progress"},
            {32, "Completed"},
            {64, "Sighting Lap"},
            {256, "Pre-Start"},
            {512, "Race Over"},
            {1024, "Completed"}
        }},
        {4, { // Straight Rhythm
            {0, "Waiting"},
            {1, "Practice"},
            {2, "Round"},
            {3, "Quarter-Finals"},
            {4, "Semi-Finals"},
            {5, "Final"},
            {16, "In Progress"},
            {32, "Completed"}
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


// Helper to convert conditions to a friendly name
std::string getFriendlyConditionsName(int condition) {
    static const std::unordered_map<int, std::string> conditionsMap = {
        {0, "Sunny"},
        {1, "Cloudy"},
        {2, "Rainy"}
    };

    auto it = conditionsMap.find(condition);
    return it != conditionsMap.end() ? it->second : "Unknown Conditions";
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

// Helper function to get custom data
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

// Mapping
const std::vector<std::pair<std::string, std::string>> Plugin::friendlyNames_ = {
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
    {"session_state", "Session State"},
    {"conditions", "Conditions"},
    {"air_temperature", "Air Temperature"}
};


// Helper function to process draw fields
void Plugin::updateDrawFields(const std::unordered_map<std::string, std::string>& fields) {
    std::ostringstream logStream;

    logStream << "Selected Data:\n";

    std::vector<std::string> tempDisplayStrings;

    if (configManager_.getValue("plugin_banner") == "true") {
        tempDisplayStrings.emplace_back(PLUGIN_VERSION); // truncate string
    }

    for (const auto& [configKey, friendlyName] : friendlyNames_) {
        auto it = fields.find(friendlyName);
        if (it != fields.end()) {
            const std::string& fieldValue = it->second;

            // Check the config to see if this field should be logged/drawn
            if (configManager_.getValue(configKey) == "true") {
                logStream << friendlyName << ": " << fieldValue << "\n";
                tempDisplayStrings.emplace_back(friendlyName + ": " + fieldValue.substr(0, 32)); // truncate string
            }
        }
    }

    // Update the display strings
    {
        std::lock_guard<std::mutex> guard(displayMutex_);
        displayStrings_ = tempDisplayStrings;
    }
}

void Plugin::onEventInit(const SPluginsBikeEvent_t& eventData) {
    Logger::getInstance().log("EventInit handler triggered");

    // Store EventInit data
    riderName_ = eventData.m_szRiderName;
    bikeID_ = eventData.m_szBikeID;
    bikeName_ = eventData.m_szBikeName;
    category_ = eventData.m_szCategory;
    trackID_ = eventData.m_szTrackID;
    trackName_ = eventData.m_szTrackName;
    trackLength_ = eventData.m_fTrackLength;
    type_ = eventData.m_iType;

    // Get Necessary custom data to determine connection state
    localServerName_ = getCustomData("local_server_name_offset", "local_server_name_size", "local_server_name");
    remoteServerIP_ = getCustomData("remote_server_ip_offset", "remote_server_ip_size", "remote_server_ip");

    // Get the rest of the custom data
    if (!localServerName_.empty()) { // is host
        connectionType_ = "Host";
        serverName_ = localServerName_;
        serverPassword_ = getCustomData("local_server_password_offset", "local_server_password_size", "local_server_password");
    }
    else if (remoteServerIP_ != "0.0.0.0") { // is online
        connectionType_ = "Client";
        serverPassword_ = getCustomData("remote_server_password_offset", "remote_server_password_size", "remote_server_password");
        remoteServerPort_ = getCustomData("remote_server_port_offset", "remote_server_port_size", "remote_server_port");

        // Only prepare- and perform search if Server Name is enabled
        if (configManager_.getValue("server_name") == "true") {
            // Search for server name
            std::string searchPattern(reinterpret_cast<char*>(rawRemoteServer_.data()), rawRemoteServer_.size());

            // Get remote server name size and offset from configuration
            size_t remoteServerNameSize = std::stoul(configManager_.getValue("remote_server_name_size"));
            size_t remoteServerNameOffset = std::stoul(configManager_.getValue("remote_server_name_offset"));

            serverName_ = memReader_.searchMemory(searchPattern, remoteServerNameSize, remoteServerNameOffset);
        }

    }
    else {
        connectionType_ = "Offline";
        serverName_ = "Testing";
    }
}



// RunInit
void Plugin::onRunInit(const SPluginsBikeSession_t& sessionData) {
    Logger::getInstance().log("RunInit handler triggered");

    // Populate fields with friendly names as keys
    std::unordered_map<std::string, std::string> drawFields = {
        {"Rider Name", riderName_},
        {"Category", category_},
        {"Bike ID", bikeID_},
        {"Bike Name", bikeName_},
        {"Setup", std::strlen(sessionData.m_szSetupFileName) > 0 ? std::string(sessionData.m_szSetupFileName).substr(1) : "Default"},
        {"Track ID", trackID_},
        {"Track Name", trackName_},
        {"Track Length", std::to_string(static_cast<int>(std::round(trackLength_))) + " m"},
        {"Connection", connectionType_},
        {"Event Type", getFriendlyEventTypeName(type_)},
        {"Session State", getFriendlySessionStateName(type_, sessionData.m_iSession)},
        {"Conditions", getFriendlyConditionsName(sessionData.m_iConditions)},
        {"Air Temperature", std::to_string(static_cast<int>(std::round(sessionData.m_fAirTemperature))) + "°C"}
    };

    if (connectionType_ != "Offline") {
        drawFields["Server Name"] = serverName_;
        if (!serverPassword_.empty()) {
            drawFields["Server Password"] = serverPassword_;
        }
    }

    // Process fields
    updateDrawFields(drawFields);
}

// EventDeinit
void Plugin::onEventDeinit() {
    Logger::getInstance().log("Plugin handling EventDeinit");
    localServerName_.clear();
    remoteServerIP_.clear();
    remoteServerPort_.clear();
    rawRemoteServer_.clear();
    serverName_.clear();
    serverPassword_.clear();
    connectionType_.clear();

    // Clear the display strings as well
    {
        std::lock_guard<std::mutex> guard(displayMutex_);
        displayStrings_.clear();
    }
}
