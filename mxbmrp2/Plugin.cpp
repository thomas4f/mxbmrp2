
// Plugin.cpp

#include "pch.h"

#include <filesystem>
#include <string_view>
#include <cmath>
#include <cstring>
#include <mutex>

#include "Constants.h"
#include "Plugin.h"
#include "PluginHelpers.h"
#include "Logger.h"
#include "MemReader.h"
#include "MemReaderHelpers.h"
#include "KeyPressHandler.h"
#include "timeTracker.h"

#pragma comment(lib, "ws2_32.lib")

// Singleton instance
Plugin& Plugin::getInstance() {
	static Plugin instance;
	return instance;
}

// Constructor
Plugin::Plugin()
	: configManager_(ConfigManager::getInstance()),
	  memReader_(MemReader::getInstance()) {
}

// Destructor
Plugin::~Plugin() {
	Logger::getInstance().log("Plugin instance destroyed");
}

// Initialize the plugin and other classes
void Plugin::onStartup(const std::string& savePath) {

	Logger::getInstance().setLogFileName(std::filesystem::path(savePath) / LOG_FILE);
	Logger::getInstance().log("Initializing " + std::string(PLUGIN_VERSION) + " for " + std::string(HOST_VERSION));

	// Initialize ConfigManager
	configPath_ = std::filesystem::path(savePath) / CONFIG_FILE;
	configManager_.loadConfig(configPath_);

	// Initialize MemReader
	memReader_.initialize();

	// Initialize displayEnabled_ from hud settings
	displayEnabled_ = configManager_.getValue<bool>("default_enabled");

	// Now that ConfigManager is loaded, load Draw configuration
	setDisplayConfig();

	// Initialize KeyPressHandler with the toggleDisplay callback
	keyPressHandler_ = std::make_unique<KeyPressHandler>([this]() { this->toggleDisplay(); }, HOTKEY);

	// timeTracker
	TimeTracker::getInstance().initialize(std::filesystem::path(savePath) / TIME_TRACKER_FILE);

	// Start the periodic task thread
	runPeriodicTask_ = true;
	periodicTaskThread_ = std::thread(&Plugin::periodicTaskLoop, this);
}

// Shutdown the plugin
void Plugin::onShutdown() {
	std::lock_guard<std::mutex> lk(mutex_);

	Logger::getInstance().log("Plugin shutting down");
	keyPressHandler_.reset();
	runPeriodicTask_ = false;

	if (periodicTaskThread_.joinable())
		periodicTaskThread_.join();

	// Catch ALT-F4 (since onRunStop/onRunDeinit isn't called then)
	if (!bikeID_.empty() && !trackID_.empty()) {
		TimeTracker::getInstance().endRun(trackID_, bikeID_);
		TimeTracker::getInstance().save();
	}
}

// Periodic tasks
void Plugin::periodicTaskLoop() {

	Logger::getInstance().log("Periodic task thread started with interval: " + std::to_string(PERIODIC_TASK_INTERVAL) + " ms"
	);

	while (runPeriodicTask_) {
		{
			std::lock_guard<std::mutex> lk(mutex_);

			if (connectionType_ == "Client") {
				serverPing_ = MemReaderHelpers::getRemoteServerPing();
			}
			if (connectionType_ == "Host" || connectionType_ == "Client") {
				serverClients_ = MemReaderHelpers::getServerClientsCount();
				
				updateDataKeys({
					{"server_ping", serverPing_},
					{"server_clients", serverClients_ + "/" + serverClientsMax_}
				});
			}

			updateDataKeys({
				{"combo_time", TimeTracker::getInstance().getCurrentComboTime()},
				{"total_time", TimeTracker::getInstance().getTotalTime()}
			});
		}
		std::this_thread::sleep_for(
			std::chrono::milliseconds(PERIODIC_TASK_INTERVAL)
		);
	}

	Logger::getInstance().log("Periodic task thread stopped");
}

// Set Draw-related config values
void Plugin::setDisplayConfig() {
	// NOTE: this function is NOT thread-safe on its own!
	displayConfig_.fontName = (std::filesystem::path(DATA_DIR) / configManager_.getValue<std::string>("font_name")).string();
	displayConfig_.fontSize = configManager_.getValue<float>("font_size");
	displayConfig_.lineHeight = displayConfig_.fontSize * LINE_HEIGHT_MULTIPLIER;
	displayConfig_.fontColor = configManager_.getValue<unsigned long>("font_color");
	displayConfig_.backgroundColor = configManager_.getValue<unsigned long>("background_color");
	displayConfig_.positionX = configManager_.getValue<float>("position_x");
	displayConfig_.positionY = configManager_.getValue<float>("position_y");
	displayConfig_.quadWidth = (displayConfig_.fontSize * MAX_STRING_LENGTH) / 4 + (displayConfig_.fontSize / 4);
}

// updateDataKeys
void Plugin::updateDataKeys(const std::unordered_map<std::string, std::string>& dataKeys) {
	// NOTE: this function is NOT thread-safe on its own!

	// Merge in the new values
	for (const auto& [key, value] : dataKeys) {
		allDataKeys_[key] = value;
	}

	// Rebuild the display list
	auto newDisplay = PluginHelpers::buildDisplayStrings(
		allDataKeys_,
		configKeyToDisplayNameMap,
		configManager_,
		MAX_STRING_LENGTH
	);

	dataKeysToDisplay_ = std::move(newDisplay);
}

// Public getter for keys to display
std::vector<std::string> Plugin::getDisplayKeys() {
	std::lock_guard<std::mutex> lk(mutex_);
	return displayEnabled_
		? dataKeysToDisplay_
		: std::vector<std::string>{};
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
	{"server_clients", "Server Clients"},
	{"event_type", "Event Type"},
	{"session_type", "Session Type"},
	{"session_state", "Session State"},
	{"conditions", "Conditions"},
	{"air_temperature", "Air Temperature"},
	{"combo_time", "Combo Track Time"},
	{"total_time", "Total Track Time"}
};

// stateChange
void Plugin::onStateChange(int gameState) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = PluginHelpers::getGameState(gameState);
	Logger::getInstance().log(playerActivity_);
}

// EventInit
void Plugin::onEventInit(const SPluginsBikeEvent_t& eventData) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	eventType_ = eventData.m_iType;
	trackID_ = eventData.m_szTrackID;
	bikeID_ = eventData.m_szBikeID;

	// Should not have changed since last onracesession ... 
	//Logger::getInstance().log(playerActivity_);

	updateDataKeys({
		{"rider_name", eventData.m_szRiderName},
		{"bike_category", eventData.m_szCategory},
		{"bike_id", eventData.m_szBikeID},
		{"bike_name", eventData.m_szBikeName},
		{"track_id", eventData.m_szTrackID},
		{"track_name", eventData.m_szTrackName},
		{"track_length", std::to_string(std::lround(eventData.m_fTrackLength)) + " m"},
		{"event_type", PluginHelpers::getEventType(eventData.m_iType, connectionType_)}
	});
}

// RunInit - To Track
void Plugin::onRunInit(const SPluginsBikeSession_t& sessionData) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = "On Track";
	TimeTracker::getInstance().startRun(trackID_, bikeID_);

	Logger::getInstance().log(playerActivity_);

	updateDataKeys({
		{"setup_name", std::strlen(sessionData.m_szSetupFileName) > 0 ? std::string(sessionData.m_szSetupFileName).substr(1) : "Default" },
		{"session_type", PluginHelpers::getSessionType(eventType_, sessionData.m_iSession)},
		{"conditions", PluginHelpers::getConditions(sessionData.m_iConditions)},
		{"air_temperature", std::to_string(std::lround(sessionData.m_fAirTemperature)) + "°C"}
	});
}

// RunDeInit - Return to Pit
void Plugin::onRunDeinit() {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = "In Pits";
	Logger::getInstance().log(playerActivity_);
	TimeTracker::getInstance().endRun(trackID_, bikeID_);
}

// RaceSession
void Plugin::onRaceSession(const SPluginsRaceSession_t& raceSession) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = "In Pits";
	Logger::getInstance().log(playerActivity_);

	// Identify the connection type
	std::string localServerName = MemReaderHelpers::getLocalServerName();

	if (!localServerName.empty()) {
		// Definitely a Host
		connectionType_ = "Host";
		serverName_ = localServerName;
	}
	else {
		auto remoteServerSocketAddress_ = MemReaderHelpers::getRemoteServerSocketAddress();
		remoteServerIPv6Address_ = PluginHelpers::formatIPv6MappedIPv4(remoteServerSocketAddress_);

		if (!remoteServerSocketAddress_.empty()) { // Definitely a Client
			connectionType_ = "Client";
			serverPassword_ = MemReaderHelpers::getRemoteServerPassword();

			std::string connectURIServerName = PluginHelpers::getServerNameFromConnectURI(MemReaderHelpers::getConnectURIString());
			if (!connectURIServerName.empty()) { // Client is using connect URI
				serverName_ = connectURIServerName;

				// Can't get data from server list when using connect URI :(
				serverLocation_ = "?";
				serverClientsMax_ = "?";
			}
			else { // Client connected normally
				std::tie(remoteServerIPv6AddressMemoryAddress_, serverName_) =
					MemReaderHelpers::getRemoteServerNameAndAddress(remoteServerSocketAddress_);
				if (serverName_.empty()) serverName_ = "Unknown";

				serverLocation_ = MemReaderHelpers::getRemoteServerLocation(remoteServerIPv6AddressMemoryAddress_);
				serverClientsMax_ = MemReaderHelpers::getRemoteServerClientsMax(remoteServerIPv6AddressMemoryAddress_);
			}
		}
		else {
			connectionType_ = "Offline";
		}
	}

	if (connectionType_ == "Client") {
		std::string connectURIString = PluginHelpers::buildConnectURIString(
			remoteServerIPv6Address_,
			serverName_,
			serverPassword_,
			MemReaderHelpers::getServerTrackID(),
			MemReaderHelpers::getServerCategories()
		);

		Logger::getInstance().log("connectURIString:\n" + connectURIString);
	}

	if (connectionType_ == "Host") {
		serverPassword_ = MemReaderHelpers::getLocalServerPassword();
		serverLocation_ = MemReaderHelpers::getLocalServerLocation();
		serverClientsMax_ = MemReaderHelpers::getLocalServerClientsMax();
	}

	updateDataKeys({
		{"plugin_banner", PLUGIN_VERSION},
		{"server_name", serverName_},
		{"server_password", serverPassword_},
		{"server_location", serverLocation_},
		{"connection_type", connectionType_},
		{"session_state", PluginHelpers::getSessionState(raceSession.m_iSessionState)}
	});
}

// RaceSessionState
void Plugin::onRaceSessionState(const SPluginsRaceSessionState_t& raceSessionState) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	updateDataKeys({ {"session_state", PluginHelpers::getSessionState(raceSessionState.m_iSessionState)} });
}

// RaceAddEntry
void Plugin::onRaceAddEntry(const SPluginsRaceAddEntry_t& raceAddEntry) {
	std::lock_guard<std::mutex> lk(mutex_);
	//Logger::getInstance().log(std::string(__func__) + " handler triggered");

	// Check whether the entry is in fact the local player
	if (std::string_view{ raceAddEntry.m_szName } == allDataKeys_["rider_name"] &&
		std::string_view{ raceAddEntry.m_szBikeName } == allDataKeys_["bike_name"])
	{
		updateDataKeys({ {"race_number", std::to_string(raceAddEntry.m_iRaceNum)} });
	}
}

// RaceRemoveEntry
void Plugin::onRaceRemoveEntry(const SPluginsRaceRemoveEntry_t& raceRemoveEntry) {
	//Logger::getInstance().log(std::string(__func__) + " handler triggered");
}

// EventDeinit
void Plugin::onEventDeinit() {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered\n");

	TimeTracker::getInstance().save();

	playerActivity_ = DEFAULT_PLAYER_ACTIVITY;
	eventType_ = -1;
	trackID_.clear();
	bikeID_.clear();
	remoteServerIPv6Address_.clear();
	remoteServerIPv6AddressMemoryAddress_ = 0;
	serverName_.clear();
	serverPassword_.clear();
	connectionType_.clear();
	serverLocation_.clear();
	serverPing_.clear();
	serverClients_.clear();
	serverClientsMax_.clear();

	allDataKeys_.clear();
	dataKeysToDisplay_.clear();

	Logger::getInstance().log(playerActivity_);
}

// RunStart - Pause/Unpause
void Plugin::onRunStart() {
	Logger::getInstance().log(std::string(__func__) + " handler triggered");
	// Not needed yet.
}

// RunStop - Pause/Unpause
void Plugin::onRunStop() {
	Logger::getInstance().log(std::string(__func__) + " handler triggered");
	// Not needed yet.
}

// Define the KeyPressHandler callback function
void Plugin::toggleDisplay() {
	std::lock_guard<std::mutex> lk(mutex_);

	displayEnabled_ = !displayEnabled_;

	if (displayEnabled_) {
		Logger::getInstance().log("Display enabled.");

		// Reload configuration and update display settings
		configManager_.loadConfig(configPath_);
		setDisplayConfig();

		// Re-populate displayKeysToDisplay_ based on current allDataKeys_
		updateDataKeys(allDataKeys_);
	}
	else {
		Logger::getInstance().log("Display disabled.");
	}
}
