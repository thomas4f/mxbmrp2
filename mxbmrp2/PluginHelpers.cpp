
// PluginHelpers.cpp

#include "pch.h"

#include <cstring>
#include <sstream>

#include "PluginHelpers.h"
#include "ConfigManager.h"

namespace PluginHelpers {

    std::vector<std::string> buildDisplayStrings(
        const std::unordered_map<std::string, std::string>& allDataKeys,
        const std::vector<std::pair<std::string, std::string>>& configKeyToDisplayNameMap,
        ConfigManager& configManager,
        size_t maxLength)
    {
        std::vector<std::string> displayKeysBuffer;

        auto isEnabled = [&configManager](const std::string& key) {
            return configManager.getValue<bool>(key);
            };

        for (const auto& [configKey, displayName] : configKeyToDisplayNameMap) {
            if (isEnabled(configKey)) {
                auto it = allDataKeys.find(configKey);
                if (it != allDataKeys.end() && !it->second.empty()) {
                    std::string displayString;

                    if (configKey == "plugin_banner") {
                        displayString = it->second;
                    }
                    else if (configKey == "rider_name" &&
                        isEnabled("race_number") &&
                        allDataKeys.count("race_number") &&
                        !allDataKeys.at("race_number").empty())
                    {
                        displayString = displayName + ": " +
                            allDataKeys.at("race_number") + " " + it->second;
                    }
                    else {
                        displayString = displayName + ": " + it->second;
                    }

                    displayKeysBuffer.emplace_back(displayString.substr(0, maxLength));
                }
            }
        }

        return displayKeysBuffer;
    }

    std::string getGameState(int gameState) {
        static const std::unordered_map<int, std::string> gameStateMap = {
            {-1, "In Menus"},
            {0, "On Track"},
            {1, "Spectating"},
            {2, "Watching Replay"}
        };

        auto it = gameStateMap.find(gameState);
        return it != gameStateMap.end() ? it->second : "Unknown";
    }

    std::string getEventType(int type, const std::string& connectionType) {
        static const std::unordered_map<int, std::string> eventTypeNames = {
            {1, "Testing"},
            {2, "Race"},
            {4, "Straight Rhythm"}
        };

        // Edge case: Testing when online is called Open Practice
        if (type == 1 && connectionType != "Offline") {
            return "Open Practice";
        }

        auto it = eventTypeNames.find(type);
        return it != eventTypeNames.end() ? it->second : "Unknown";
    }

    std::string getSessionType(int type, int session) {
        static const std::unordered_map<int, std::unordered_map<int, std::string>> sessionStateNames = {
            {1, { // Testing / Open Practice
                {0, "Waiting"},
                {1, "Practice"}
            }},
            {3, { // Qualify Practice}
                {0, "Waiting"}
            }},
            {2, { // Race
                {0, "Waiting"},
                {1, "Practice"},
                {2, "Pre-Qualify"},
                {3, "Unknown"}, // Not Used?
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

        if (auto typeIt = sessionStateNames.find(type); typeIt != sessionStateNames.end()) {
            if (auto sessionIt = typeIt->second.find(session); sessionIt != typeIt->second.end()) {
                return sessionIt->second;
            }
        }
        return "Unknown";
    }

    std::string getSessionState(int sessionState) {
        static const std::unordered_map<int, std::string> sessionStateNames = {
            {0, "In Progress"},
            {16, "In Progress"},
            {32, "Completed"},
            {64, "Sighting Lap"},
            {256, "Pre-Start"},
            {512, "Race Over"},
            {1024, "Completed"},
            {2048, "Completed"}
        };

        auto it = sessionStateNames.find(sessionState);
        return it != sessionStateNames.end() ? it->second : "Unknown";
    }

    std::string getConditions(int condition) {
        static const std::unordered_map<int, std::string> conditionsMap = {
            {0, "Clear"},
            {1, "Cloudy"},
            {2, "Rainy"}
        };

        auto it = conditionsMap.find(condition);
        return it != conditionsMap.end() ? it->second : "Unknown";
    }

    std::string formatIPv6MappedIPv4(const ByteBuf& ipv6) {
        // Need at least: 2×0xFF + 4 IPv4 bytes + 2 port bytes
        if (ipv6.size() < 8) {
            return "Invalid";
        }

        uint16_t hext1, hext2, port;
        // copy the raw bytes (network order) into our words
        std::memcpy(&hext1, ipv6.data() + 2, sizeof(hext1));  // first half of IPv4
        std::memcpy(&hext2, ipv6.data() + 4, sizeof(hext2));  // second half of IPv4
        std::memcpy(&port, ipv6.data() + 6, sizeof(port));   // port

        // convert from network byte order to host
        hext1 = ntohs(hext1);
        hext2 = ntohs(hext2);
        port = ntohs(port);

        std::ostringstream oss;
        oss << "[::ffff:"
            << std::hex << hext1 << ":" << hext2
            << std::dec << "]:" << port
            << "/[::1]:" << port;
        return oss.str();
    }

    // mxbikes.exe -connect "mxbikes://ip:port/local_ip:local_port/server name/password/track_id/layout_id/categories"
    std::string buildConnectURIString(const std::string& formattedIPv6,
        const std::string& serverName,
        const std::string& password,
        const std::string& trackID,
        const std::string& serverCategories)
    {
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string passwordFlag = password.empty() ? "0" : "1";

        return "\"" + std::string(exePath) + "\"" + " -connect \"mxbikes://" +
            formattedIPv6 + "/" +
            serverName + "/" +
            passwordFlag + "/" +
            trackID +
            "/0/" +
            serverCategories + "\"";
    }

    std::string getServerNameFromConnectURI(const std::string& connStr) {
        std::istringstream ss(connStr);
        std::string token;

        // We need the 5th token (index 4)
        for (int i = 0; i < 5; ++i) {
            if (!std::getline(ss, token, '/')) {
                return {};
            }
        }

        return token;
    }

    std::string getSessionDuration(int numLaps, int sessionLenMs, int sessionTimeMs) {
        // ms -> mm:ss
        auto toClock = [](int ms) -> std::string {
            if (ms < 0) ms = 0;
            int total = ms / 1000;
            std::ostringstream oss;
            oss << std::setw(2) << std::setfill('0') << (total / 60)
                << ':'
                << std::setw(2) << std::setfill('0') << (total % 60);
            return oss.str();
            };

		// time-only session (testing/open practice)
        if (numLaps <= 0)
            return toClock(sessionTimeMs);

        // lap-only race
        if (sessionLenMs == 0)
            return numLaps == 1 ? "1 Lap" : std::to_string(numLaps) + " Laps";

        // time + lap race
        return toClock(sessionTimeMs) + " +" + (numLaps == 1 ? "1 Lap" : std::to_string(numLaps) + " Laps");
    }
}
