
// DiscordManager.h

#pragma once

#include <memory>
#include <cstdint>
#include <chrono>
#include <string>

#include "discord.h"

class DiscordManager {
public:
    DiscordManager();
    ~DiscordManager();

    // Start or restart the client
    bool initialize(uint64_t appId);

    // Pump callbacks, update presence, and possibly reconnect
    void tick(
        const std::string& details,
        const std::string& state,
        int partySize = 0,
        int partyMax = 0);

    // Complete shutdown
    void finalize();

    /// Returns status
    std::string getConnectionStateString() const;

private:
    // Internal helper to create the Core object
    bool createCore();

    // Called whenever connection is broken
    void scheduleReconnect(discord::Result why);

    std::unique_ptr<discord::Core> core_;
    uint64_t appId_{ 0 };

    // reconnect state machine
    enum class ConnectionState {
        Disconnected,
        Waiting,
        Connected,
        LoadFailed
    };
    ConnectionState connState_{ ConnectionState::Disconnected };

    int backoffSec_{ 5 };
    std::chrono::steady_clock::time_point nextAttempt_;
    bool allowReconnect_{ false };

    // Timestamp
    std::int64_t startTimestamp_{ 0 };
};
