
// Plugin.h

#pragma once

#include <mutex>
#include <memory> 
#include "MXB_interface.h"
#include "ConfigManager.h"
#include "MemReader.h"
#include "KeyPressHandler.h"

class Plugin {
public:
    static Plugin& getInstance();

    void initialize();
    void shutdown();

    // MXB Interface
    void onEventInit(const SPluginsBikeEvent_t& eventData);
    void onEventDeinit();
    void onRunInit(const SPluginsBikeSession_t& sessionData);
    void onRaceSession(const SPluginsRaceSession_t& raceSession);
    void onRaceSessionState(const SPluginsRaceSessionState_t& raceSessionState);
    void onRaceAddEntry(const SPluginsRaceAddEntry_t& raceAddEntry);

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

    // Cached references to singleton instances
    ConfigManager& configManager_;
    MemReader& memReader_;

    // KeyPressHandler instance
    std::unique_ptr<KeyPressHandler> keyPressHandler_;

    // Callback function to toggle display
    void toggleDisplay();

    // Mutex for thread-safe operations
    std::mutex mutex_;

    // Helper to read memory
    std::string readMemString(
        bool relative,
        const std::string& offsetKey,
        const std::string& sizeKey,
        const std::string& logKey,
        bool nullTerminated
    );

    // Helper to process and update draw fields
    void updateDataKeys(const std::unordered_map<std::string, std::string>& fields);

    // Method to load Draw-related config values
    void setDisplayConfig();

    // Display name helpers
    std::string getEventTypeDisplayName(int type, const std::string& connectionType);
    std::string getSessionDisplayName(int type, int session);
    std::string getSessionStateDisplayName(int sessionState);
    std::string getConditionsDisplayName(int condition);

    // Custom data keys
    int type_ = 0;
    std::string serverName_ = "";
    std::string serverPassword_ = "";
    std::string serverLocation_ = "";
    std::string localServerName_ = "";
    std::string localServerPassword_ = "";
    std::string remoteServerIP_ = "";
    std::string connectionType_ = "";

    std::vector<unsigned char> rawRemoteServer_;

    // Stores all key-value pairs of data for processing and display.
    std::unordered_map<std::string, std::string> allDataKeys_;

    // Maps internal configuration keys to user-friendly display names.
    static const std::vector<std::pair<std::string, std::string>> configKeyToDisplayNameMap;

    // Holds the final set of data intended for display in the plugin's user interface
    std::vector<std::string> dataKeysToDisplay_;

    // Display state flag
    bool displayEnabled_ = true;
};
