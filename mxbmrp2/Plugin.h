
// Plugin.h

#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

#include "MXB_interface.h"
#include "ConfigManager.h"
#include "MemReader.h"
#include "KeyPressHandler.h"
#include "Constants.h"

class Plugin {
public:
    static Plugin& getInstance();

    mutable std::mutex mutex_;  // protect all non-atomic shared state

    // MXB Interface
    void onStartup(const std::string& savePath);
    void onEventInit(const SPluginsBikeEvent_t& eventData);
    void onEventDeinit();
    void onRunInit(const SPluginsBikeSession_t& sessionData);
    void onRunDeinit();
    void onRaceSession(const SPluginsRaceSession_t& raceSession);
    void onRaceSessionState(const SPluginsRaceSessionState_t& raceSessionState);
    void onRaceAddEntry(const SPluginsRaceAddEntry_t& raceAddEntry);
    void onRaceRemoveEntry(const SPluginsRaceRemoveEntry_t& raceRemoveEntry);
    void onStateChange(int state);
    void onRunStart();
    void onRunStop();
    void onShutdown();

    // Method to retrieve the keys to display
    std::vector<std::string> getDisplayKeys();

    // Cached configuration values for Draw
    struct displayConfig {
        std::string fontName;
        float fontSize;
        float lineHeight;
        unsigned long fontColor;
        unsigned long backgroundColor;
        float positionX;
        float positionY;
        float quadWidth;
    } displayConfig_;

private:
    Plugin();
    ~Plugin();

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    // Path to the config file
    std::filesystem::path configPath_;

    // Cached references to singleton instances
    ConfigManager& configManager_;
    MemReader& memReader_;

    // KeyPressHandler instance
    std::unique_ptr<KeyPressHandler> keyPressHandler_;

    // Helper to process and update draw fields
    void updateDataKeys(const std::unordered_map<std::string, std::string>& fields);

    // Method to load Draw-related config values
    void setDisplayConfig();

    // Custom data keys
    std::string playerActivity_ = DEFAULT_PLAYER_ACTIVITY;
    int eventType_ = -1;
    std::string trackID_ = "";
    std::string bikeID_ = "";
    std::string remoteServerIPv6Address_ = "";
    uintptr_t remoteServerIPv6AddressMemoryAddress_ = 0;
    std::string serverName_ = "";
    std::string serverPassword_ = "";
    std::string connectionType_ = "";
    std::string serverLocation_ = "";
    std::string serverPing_ = "";
    std::string serverClients_ = "";
    std::string serverClientsMax_ = "";

    // Stores all key-value pairs of data for processing and display.
    std::unordered_map<std::string, std::string> allDataKeys_;

    // Maps internal configuration keys to user-friendly display names.
    static const std::vector<std::pair<std::string, std::string>> configKeyToDisplayNameMap;

    // Holds the final set of data intended for display in the plugin's user interface
    std::vector<std::string> dataKeysToDisplay_;

    // Display state flag
    bool displayEnabled_ = true;

    // Periodic tasks
    std::thread periodicTaskThread_;
    std::atomic<bool> runPeriodicTask_{ true };
    void periodicTaskLoop();

    // Callback function to toggle display 
    void toggleDisplay();
};
