
// Plugin.cpp

#include "pch.h"
#include "Constants.h"
#include "Plugin.h"
#include "Logger.h"
#include "MemReader.h"
#include "KeyPressHandler.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

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

    // Initialize displayEnabled_ from enable_hud config
    displayEnabled_ = configManager_.getValue<bool>("default_enabled");

    // Now that ConfigManager is loaded, load Draw configuration
    setDisplayConfig();

    // Initialize KeyPressHandler with the toggleDisplay callback
    keyPressHandler_ = std::make_unique<KeyPressHandler>([this]() { this->toggleDisplay(); }, HOTKEY);
    Logger::getInstance().log("KeyPressHandler initialized with hotkey CTRL + " + std::string(1, static_cast<char>(HOTKEY)));

    // Start the periodic task thread
    stopPeriodicTask_ = false;
    periodicTaskThread_ = std::thread(&Plugin::periodicTaskLoop, this);
}

// Shutdown the plugin
void Plugin::shutdown() {
    Logger::getInstance().log("Plugin shutting down");

    // Signal the periodic task thread to stop
    stopPeriodicTask_ = true;
    if (periodicTaskThread_.joinable()) {
        periodicTaskThread_.join();
    }
}

// Periodic tasks
void Plugin::periodicTaskLoop() {
    Logger::getInstance().log("Periodic task thread started");

    while (!stopPeriodicTask_) {
        if (connectionType_ == "Client") {
            std::string remoteServerPingHex = memReader_.readStringAtAddress(
                true,
                configManager_.getValue<unsigned long>("remote_server_ping_offset"),
                configManager_.getValue<unsigned long>("remote_server_ping_size"),
                "remote_server_ping",
                false
            );

            // Extract the two bytes and combine them to form a little-endian integer
            unsigned int remoteServerPing = static_cast<unsigned char>(remoteServerPingHex[0]) |
                (static_cast<unsigned char>(remoteServerPingHex[1]) << 8);

            updateDataKeys({ {"server_ping", remoteServerPing ? std::to_string(remoteServerPing) + " ms" : "N/A"} });
        }

        // Sleep for the configured interval
        std::this_thread::sleep_for(std::chrono::milliseconds(PERIODICTASKINTERVAL));
    }

    Logger::getInstance().log("Periodic task thread stopped.");
}

// Load Draw-related config values
void Plugin::setDisplayConfig() {
    displayConfig_.fontName = std::string(DATA_DIR) + configManager_.getValue<std::string>("font_name");
    displayConfig_.fontSize = configManager_.getValue<float>("font_size");
    displayConfig_.lineHeight = displayConfig_.fontSize * 1.1f;
    displayConfig_.fontColor = configManager_.getValue<unsigned long>("font_color");
    displayConfig_.backgroundColor = configManager_.getValue<unsigned long>("background_color");
    displayConfig_.positionX = configManager_.getValue<float>("position_x");
    displayConfig_.positionY = configManager_.getValue<float>("position_y");
    displayConfig_.quadWidth = (displayConfig_.fontSize * MAX_STRING_LENGTH) / 4 + (displayConfig_.fontSize / 4);
}

// updateDataKeys
void Plugin::updateDataKeys(const std::unordered_map<std::string, std::string>& dataKeys) {
    std::lock_guard<std::mutex> guard(mutex_);

    // Update or add keys from the input map to allDataKeys_
    for (const auto& [key, value] : dataKeys) {
        allDataKeys_[key] = value;
    }

    // Prepare a temporary buffer for data keys to be displayed
    std::vector<std::string> displayKeysBuffer;

    // Inline utility to check if a config key is enabled
    auto isEnabled = [this](const std::string& cfgKey) -> bool {
        return configManager_.getValue<bool>(cfgKey);
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
                }
                else if (configKey == "rider_name" && isEnabled("race_number") && !allDataKeys_["race_number"].empty()) {
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
    return displayEnabled_ ? dataKeysToDisplay_ : std::vector<std::string>{};
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
            {3, "?"}, // Not Used?
            {4, "Qualify"},
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
        {0, ""}, // Empty in Testing and Open Practice
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

// Helper to convert IP to a human-readable format
std::string toIPv4(const std::string& rawData) {
    std::ostringstream oss;
    for (size_t i = 0; i < 4; ++i) {
        oss << (i ? "." : "") << static_cast<int>(static_cast<unsigned char>(rawData[i]));
    }
    return oss.str();
}

// Helper to read memory
std::string Plugin::readMemString(
    bool relative,
    const std::string& offsetKey,
    const std::string& sizeKey,
    const std::string& logKey,
    bool nullTerminated
) {
    const auto offset = configManager_.getValue<unsigned long>(offsetKey);
    const auto size = configManager_.getValue<unsigned long>(sizeKey);

    return memReader_.readStringAtAddress(relative, offset, size, logKey, nullTerminated);
}

// Maps config keys to display names and sets display order
const std::vector<std::pair<std::string, std::string>> Plugin::configKeyToDisplayNameMap = {
    {"plugin_banner", "Plugin Banner"},
    {"rider_name", "Rider Name"},
    {"bike_category", "Bike Category"},
    {"bike_id", "Bike ID"},
    {"bike_name", "Bike Name"},
    {"setup_name", "Setup Name"},
    {"track_id", "Track ID"},
    {"track_name", "Track Name"},
    {"track_length", "Track Length"},
    {"connection_type", "Connection Type"},
    {"server_name", "Server Name"},
    {"server_password", "Server Password"},
    {"server_location", "Server Location"},
    {"server_ping", "Server Ping"},
    {"event_type", "Event Type"},
    {"session_type", "Session Type"},
    {"session_state", "Session State"},
    {"conditions", "Conditions"},
    {"air_temperature", "Air Temperature"}
};

// EventInit
void Plugin::onEventInit(const SPluginsBikeEvent_t& eventData) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    eventType_ = eventData.m_iType;

    // local_server_name
    localServerName_ = readMemString(
        true,
        "local_server_name_offset",
        "local_server_name_size",
        "local_server_name",
        true
    );

    // remote_server_ip
    std::string remoteServerIPAddressHex = readMemString(
        true,
        "remote_server_ip_offset",
        "remote_server_ip_size",
        "remote_server_ip",
        false // Do not null terminate
    );

    remoteServerSocketAddressHex_.insert(remoteServerSocketAddressHex_.end(), remoteServerIPAddressHex.begin(), remoteServerIPAddressHex.end());
    remoteServerIPAddress_ = toIPv4(remoteServerIPAddressHex);

    if (!localServerName_.empty()) { // Host
        serverName_ = localServerName_;
        connectionType_ = "Host";

		// local_server_password
        serverPassword_ = readMemString(
            true,
            "local_server_password_offset",
            "local_server_password_size",
            "local_server_password",
            true
        );

        // local_server_location
        serverLocation_ = readMemString(
            true,
            "local_server_location_offset",
            "local_server_location_size",
            "local_server_location",
            true
        );
    }
    else if (remoteServerIPAddress_ != "0.0.0.0") { // Client
        connectionType_ = "Client";

        // remote_server_port
        std::string remoteServerPortHex = readMemString(
            true,
            "remote_server_port_offset",
            "remote_server_port_size",
            "remote_server_port",
            false // Do not null terminate
        );

        remoteServerSocketAddressHex_.insert(remoteServerSocketAddressHex_.end(), remoteServerPortHex.begin(), remoteServerPortHex.end());

        // remote_server_password
        serverPassword_ = readMemString(
            true,
            "remote_server_password_offset",
            "remote_server_password_size",
            "remote_server_password",
            true
        );

        // Edge case: remoteServerMemoryAddress_ and remote_server_name (based on ip + port)
        std::string remoteServerName;
        std::string searchPattern(remoteServerSocketAddressHex_.begin(), remoteServerSocketAddressHex_.end());
        std::tie(remoteServerMemoryAddress_, remoteServerName) = memReader_.searchMemory(
            searchPattern,
            configManager_.getValue<unsigned long>("remote_server_name_offset"),
            configManager_.getValue<unsigned long>("remote_server_name_size"),
            "remote_server_name",
            true
        );

        serverName_ = remoteServerName;

        // Edge case: remote_server_location (relative to remoteServerMemoryAddress_)
        if (remoteServerMemoryAddress_ != 0) {
            serverLocation_ = memReader_.readStringAtAddress(
                false, // absolute address
                remoteServerMemoryAddress_ + configManager_.getValue<unsigned long>("remote_server_location_offset"),
                configManager_.getValue<unsigned long>("remote_server_location_size"),
                "remote_server_location",
                true
            );
        }
    }
    else {
        connectionType_ = "Offline";
    }

    std::unordered_map<std::string, std::string> dataKeys = {
        {"plugin_banner", PLUGIN_VERSION},
        {"rider_name", eventData.m_szRiderName},
        {"bike_category", eventData.m_szCategory},
        {"bike_id", eventData.m_szBikeID},
        {"bike_name", eventData.m_szBikeName},
        {"track_id", eventData.m_szTrackID},
        {"track_name", eventData.m_szTrackName},
        {"track_length", std::to_string(static_cast<int>(std::round(eventData.m_fTrackLength))) + " m"},
        {"server_name", serverName_},
        {"server_password", serverPassword_},
        {"server_location", serverLocation_},
        {"connection_type", connectionType_},
        {"event_type", getEventTypeDisplayName(eventData.m_iType, connectionType_)}
    };

    updateDataKeys(dataKeys);
}

// RunInit
void Plugin::onRunInit(const SPluginsBikeSession_t& sessionData) {
    Logger::getInstance().log(std::string(__func__) + " handler triggered");

    std::unordered_map<std::string, std::string> dataKeys = {
        {"setup_name", std::strlen(sessionData.m_szSetupFileName) > 0 ? std::string(sessionData.m_szSetupFileName).substr(1) : "Default"},
        {"session_type", getSessionDisplayName(eventType_, sessionData.m_iSession)},
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
    //Logger::getInstance().log(std::string(__func__) + " handler triggered");

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
    remoteServerIPAddress_.clear();
    remoteServerSocketAddressHex_.clear();
    serverName_.clear();
    serverPassword_.clear();
    connectionType_.clear();
    serverLocation_.clear();

    // Clear the display keys as well
    allDataKeys_.clear();
    dataKeysToDisplay_.clear();
}

// Define the callback function
void Plugin::toggleDisplay() {
    displayEnabled_ = !displayEnabled_;

    if (displayEnabled_) {
        Logger::getInstance().log("Display enabled.");

        // Reload configuration and update display settings
        configManager_.loadConfig("plugins\\" + std::string(DATA_DIR) + std::string(CONFIG_FILE));
        setDisplayConfig();

        // Re-populate displayKeysToDisplay_ based on current allDataKeys_
        updateDataKeys(allDataKeys_);
    }
    else {
        Logger::getInstance().log("Display disabled.");
    }
}
