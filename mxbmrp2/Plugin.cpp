
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
#include "HtmlWriter.h"

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

	// Initialize Discord Core
	useDiscordRichPresence_ = configManager_.getValue<bool>("enable_discord_rich_presence");
	if (useDiscordRichPresence_) {
		discordManager_.initialize(DISCORD_APP_ID);
	}

	updateDataKeys({ {"plugin_banner", PLUGIN_VERSION} });

	// HTML Export
	useHtmlExport_ = configManager_.getValue<bool>("enable_html_export");
	htmlPath_ = std::filesystem::path(savePath) / HTML_FILE;

	if (useHtmlExport_) {
		Logger::getInstance().log("Exporting HTML to: " + htmlPath_.string());
	}

	// Start the periodic task thread
	runPeriodicTask_ = true;
	periodicTaskThread_ = std::thread(&Plugin::periodicTaskLoop, this);

	Logger::getInstance().log(playerActivity_);
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

	// Discord
	if (useDiscordRichPresence_) {
		discordManager_.finalize();
	}

	// HTML Export
	HtmlWriter::atomicWrite(htmlPath_, HtmlWriter::renderNoData());
}

// Periodic tasks
void Plugin::periodicTaskLoop() {
	Logger::getInstance().log("Periodic task thread started with interval: " + std::to_string(PERIODIC_TASK_INTERVAL) + " ms");

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
					{"server_clients", std::to_string(serverClients_) + "/" + std::to_string(serverClientsMax_)},
					{"combo_time", TimeTracker::getInstance().getCurrentComboTime()},
					{"total_time", TimeTracker::getInstance().getTotalTime()},
					{"discord_status", discordManager_.getConnectionStateString()}
				});
			}

			// Regenerate HTML
			if (useHtmlExport_) {
				std::string html = HtmlWriter::renderHtml(
					allDataKeys_,
					configKeyToDisplayNameMap,
					configManager_);

				if (html != lastHtml_) {
					try {
						HtmlWriter::atomicWrite(htmlPath_, html);
						lastHtml_ = std::move(html);
					}
					catch (const std::exception& e) {
						Logger::getInstance().log(
							std::string("HTML write failed: ") + e.what());
					}
				}
			}
		}

		// Discord
		if (useDiscordRichPresence_) {
			std::string details = "";
			std::string state = "";
			int partySize = 0;
			int partyMax = 0;

			if (connectionType_ == "Host" || connectionType_ == "Client") {
				details = allDataKeys_["track_name"] + " (" + allDataKeys_["session_type"] + " : " + allDataKeys_["session_state"] + ")";
				state = allDataKeys_["server_name"];
				partySize = serverClients_;
				partyMax = serverClientsMax_;
			}
			else if (allDataKeys_["event_type"] == "Testing") {
				details = "Testing: " + allDataKeys_["track_name"];
			}
			else if (playerActivity_ == "In Menus") {
				details = "In Menus";
			}
			else {
				details = "Unkown";
			}

			discordManager_.tick(details, state, partySize, partyMax);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(PERIODIC_TASK_INTERVAL));
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
	displayConfig_.quadWidth = (displayConfig_.fontSize / 4) * (MAX_STRING_LENGTH + 1);
}

// updateDataKeys
void Plugin::updateDataKeys(const std::unordered_map<std::string, std::string>& dataKeys) {
	
	// Merge in the new values
	for (const auto& [key, value] : dataKeys)
		allDataKeys_[key] = value;

	// Rebuild the display list
	dataKeysToDisplay_ = PluginHelpers::buildDisplayStrings(
		allDataKeys_,
		configKeyToDisplayNameMap,
		configManager_,
		MAX_STRING_LENGTH);
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
	{"session_duration", "Session Duration"},
	{"conditions", "Conditions"},
	{"air_temperature", "Air Temperature"},
	{"track_deformation", "Track Deformation"},
	{"combo_time", "Combo Track Time"},
	{"total_time", "Total Track Time"},
	{"discord_status", "Discord RP Status"}
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
		{"event_type", PluginHelpers::getEventType(eventData.m_iType, connectionType_)}
	});
}

// RunInit - To Track
void Plugin::onRunInit(const SPluginsBikeSession_t& sessionData) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = "On Track";
	Logger::getInstance().log(playerActivity_);

	TimeTracker::getInstance().startRun(trackID_, bikeID_);

	// For highlighting the default setup
	uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	lastRunInitMs_.store(nowMs, std::memory_order_relaxed);

	updateDataKeys({
		{"setup_name", std::strlen(sessionData.m_szSetupFileName) > 0 ? std::string(sessionData.m_szSetupFileName).substr(1) : "Default" }
	});
}

// RunDeInit - Return to Pit
void Plugin::onRunDeinit() {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = "In Pits";
	Logger::getInstance().log(playerActivity_);

	TimeTracker::getInstance().endRun(trackID_, bikeID_);

	lastRunInitMs_.store(0, std::memory_order_relaxed);   // cancel highlight
}

// RaceSession
void Plugin::onRaceSession(const SPluginsRaceSession_t& raceSession) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	playerActivity_ = "In Pits";
	numLaps_ = raceSession.m_iSessionNumLaps;
	sessionLength_ = raceSession.m_iSessionLength;

	Logger::getInstance().log(playerActivity_);

	updateDataKeys({
		{"session_type", PluginHelpers::getSessionType(eventType_, raceSession.m_iSession)},
		{"session_state", PluginHelpers::getSessionState(raceSession.m_iSessionState)},
		{"conditions", PluginHelpers::getConditions(raceSession.m_iConditions)},
		{"air_temperature", std::to_string(std::lround(raceSession.m_fAirTemperature)) + " C"}
	});
}

// RaceSessionState
void Plugin::onRaceSessionState(const SPluginsRaceSessionState_t& raceSessionState) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	updateDataKeys({ {"session_state", PluginHelpers::getSessionState(raceSessionState.m_iSessionState)} });
}

// onRaceEvent
void Plugin::onRaceEvent(const SPluginsRaceEvent_t& raceEvent) {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	eventType_ = raceEvent.m_iType;

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
				serverLocation_ = "Unknown";
				serverClientsMax_ = 0;
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

		Logger::getInstance().log("connectURIString: " + connectURIString);
	}

	if (connectionType_ == "Host") {
		serverPassword_ = MemReaderHelpers::getLocalServerPassword();
		serverLocation_ = MemReaderHelpers::getLocalServerLocation();
		serverClientsMax_ = MemReaderHelpers::getLocalServerClientsMax();
	}

	updateDataKeys({
		{"server_name", serverName_},
		{"server_password", serverPassword_},
		{"server_location", serverLocation_},
		{"connection_type", connectionType_},
		{"event_type", PluginHelpers::getEventType(raceEvent.m_iType, connectionType_)},
		{"track_name", raceEvent.m_szTrackName},
		{"track_length", std::to_string(std::lround(raceEvent.m_fTrackLength)) + " m"},
		{"track_deformation", MemReaderHelpers::getTrackDeformation() + "x"}
	});
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

// RaceClassification
void Plugin::onRaceClassification(const SPluginsRaceClassification_t& raceClassification) {
	std::lock_guard<std::mutex> lk(mutex_);

	sessionTime_ = raceClassification.m_iSessionTime;

    updateDataKeys({ {"session_duration", PluginHelpers::getSessionDuration(numLaps_, sessionLength_, sessionTime_)}});
}

// EventDeinit
void Plugin::onEventDeinit() {
	std::lock_guard<std::mutex> lk(mutex_);
	Logger::getInstance().log(std::string(__func__) + " handler triggered");

	TimeTracker::getInstance().save();

	playerActivity_ = DEFAULT_PLAYER_ACTIVITY;
	eventType_ = 0;
	trackID_.clear();
	bikeID_.clear();
	remoteServerIPv6Address_.clear();
	remoteServerIPv6AddressMemoryAddress_ = 0;
	serverName_.clear();
	serverPassword_.clear();
	connectionType_.clear();
	serverLocation_.clear();
	serverPing_.clear();
	serverClients_ = 0;
	serverClientsMax_ = 0;
	numLaps_ = 0;
	sessionLength_ = 0;
	currentLap_ = 0;
	sessionTime_ = 0;

	allDataKeys_.clear();
	dataKeysToDisplay_.clear();

	updateDataKeys({ { "plugin_banner", PLUGIN_VERSION } });

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

	// Toggle the HUD on/off
	displayEnabled_ = !displayEnabled_;
	Logger::getInstance().log(
		displayEnabled_ ? "Display enabled." : "Display disabled."
	);

	// Reload everything from disk
	configManager_.loadConfig(configPath_);
	setDisplayConfig();

	// Rebuild display strings if the HUD is on
	if (displayEnabled_) {
		updateDataKeys(allDataKeys_);
	}

	// Check for config changes
	bool newUseDiscordRichPresence = configManager_.getValue<bool>("enable_discord_rich_presence");
	if (newUseDiscordRichPresence && !useDiscordRichPresence_) {
		discordManager_.initialize(DISCORD_APP_ID);
		Logger::getInstance().log("Discord Rich Presence enabled.");
	}
	else if (!newUseDiscordRichPresence && useDiscordRichPresence_) {
		discordManager_.finalize();
		Logger::getInstance().log("Discord Rich Presence disabled.");
	}
	useDiscordRichPresence_ = newUseDiscordRichPresence;

	bool newUseHtmlExport = configManager_.getValue<bool>("enable_html_export");
	if (newUseHtmlExport && !useHtmlExport_) {
		lastHtml_.clear();
		Logger::getInstance().log("HTML export enabled.");
	}
	else if (!newUseHtmlExport && useHtmlExport_) {
		Logger::getInstance().log("HTML export disabled.");
		HtmlWriter::atomicWrite(htmlPath_, HtmlWriter::renderNoData());
	}
	useHtmlExport_ = newUseHtmlExport;
}

