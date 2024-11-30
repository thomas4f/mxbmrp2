
// Plugin.h

#pragma once

#include <mutex>
#include "MXB_interface.h"
#include "ConfigManager.h"
#include "MemReader.h"

class Plugin {
public:
    static Plugin& getInstance();

    void initialize();
    void shutdown();

    void onEventInit(const SPluginsBikeEvent_t& eventData);
    void onEventDeinit();
    void onRunInit(const SPluginsBikeSession_t& sessionData);

    // Method to retrieve the display strings
    std::vector<std::string> getDisplayStrings();

    // Cached configuration values for Draw
    struct DrawConfig {
        std::string fontName;
        float fontSize;
        unsigned long fontColor;
        unsigned long backgroundColor;
        float positionX;
        float positionY;
        float quadWidth;
        unsigned long quadColor;
        // Add more as needed
    } drawConfig_;

private:
    Plugin();
    ~Plugin();

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    // Cached references to singleton instances
    ConfigManager& configManager_;
    MemReader& memReader_;

    std::string getCustomData(const std::string& keyOffset, const std::string& keySize, const std::string& label);
    void updateDrawFields(const std::unordered_map<std::string, std::string>& fields);

    // Method to load Draw-related config values
    void loadDrawConfig();

    // EventInit data members
    std::string riderName_ = "";
    std::string bikeID_ = "";
    std::string bikeName_ = "";
    std::string category_ = "";
    std::string trackID_ = "";
    std::string trackName_ = "";
    float trackLength_ = 0.0f;
    int type_ = 0;

    // Custom data members
    std::string serverName_ = "";
    std::string serverPassword_ = "";
    std::string localServerName_ = "";
    std::string localServerPassword_ = "";
    std::string remoteServerIP_ = "";
    std::string remoteServerPort_ = "";
    std::string connectionType_ = "";

    std::vector<unsigned char> rawRemoteServer_;

    // Display strings
    std::vector<std::string> displayStrings_;
    std::mutex displayMutex_; // Mutex to protect displayStrings_

    // Mapping between config keys and friendly names
    static const std::vector<std::pair<std::string, std::string>> friendlyNames_;
};
