
// DiscordManager.cpp

#include "pch.h"

#include <chrono>
#include <string>
#include <algorithm>
#include <filesystem> 

#include "DiscordManager.h"
#include "Logger.h"
#include "Constants.h"

DiscordManager::DiscordManager() = default;
DiscordManager::~DiscordManager() {
    finalize();
}

bool DiscordManager::initialize(uint64_t appId) {
    static HMODULE sdkDll = []() {
        std::filesystem::path dllPath = std::filesystem::path("plugins") / DATA_DIR / "discord_game_sdk.dll";
        return LoadLibraryW(dllPath.wstring().c_str());
        }();

    if (!sdkDll) {
        Logger::getInstance().log("Failed to load discord_game_sdk.dll, disabling Discord Rich Presence");
        connState_ = ConnectionState::LoadFailed;
        return false;
    }

    appId_ = appId;
    allowReconnect_ = true;
    connState_ = ConnectionState::Disconnected;
    backoffSec_ = 5;
    startTimestamp_ = static_cast<std::int64_t>(std::time(nullptr));

    if (!createCore()) {
        scheduleReconnect(discord::Result::InternalError);
        return false;
    }

    connState_ = ConnectionState::Connected;
    return true;
}

bool DiscordManager::createCore() {
    discord::Core* raw = nullptr;
    auto result = discord::Core::Create(
        appId_,
        DiscordCreateFlags_NoRequireDiscord,
        &raw
    );
    if (result != discord::Result::Ok || !raw) {
        Logger::getInstance().log(
            "Discord Core creation failed: " +
            std::to_string(static_cast<int>(result))
        );
        return false;
    }

    core_.reset(raw);
    core_->SetLogHook(discord::LogLevel::Debug,
        [](discord::LogLevel lvl, const char* msg) {
            Logger::getInstance().log(
                "[Discord] (" + std::to_string((int)lvl) + ") " + msg
            );
        }
    );
    Logger::getInstance().log("Discord Core created");
    return true;
}

void DiscordManager::tick(
    const std::string& details,
    const std::string& state,
    int partySize,
    int partyMax)
{
    using clock = std::chrono::steady_clock;

    switch (connState_) {
    case ConnectionState::Waiting: {
        auto now = clock::now();
        if (now >= nextAttempt_) {
            Logger::getInstance().log("Attempting Discord reconnect...");
            if (createCore()) {
                Logger::getInstance().log("Reconnected to Discord");
                backoffSec_ = 5;
                connState_ = ConnectionState::Connected;
            }
            else {
                backoffSec_ = std::min<int>(backoffSec_ * 2, 60);
                nextAttempt_ = now + std::chrono::seconds(backoffSec_);
                Logger::getInstance().log(
                    "Reconnect failed; next attempt in " +
                    std::to_string(backoffSec_) + "s"
                );
            }
        }
        return;
    }

    case ConnectionState::Connected:
        break;

    case ConnectionState::Disconnected:
        return;
    case ConnectionState::LoadFailed:
        return;
    }


    // 3) pump callbacks
    auto cbRes = core_->RunCallbacks();
    if (cbRes != discord::Result::Ok) {
        Logger::getInstance().log(
            "Discord RunCallbacks error: " +
            std::to_string(static_cast<int>(cbRes))
        );
        scheduleReconnect(cbRes);
        return;
    }

    // 4) send an update
    discord::Activity act{};

    // Use the timestamp captured at initialize()
    act.GetTimestamps().SetStart(startTimestamp_);

    act.SetType(discord::ActivityType::Playing);

    if (!details.empty()) {
        act.SetDetails(details.c_str());
    }

    if (!state.empty()) {
        act.SetState(state.c_str());
    }

    if (partyMax > 0) {
        act.GetParty().GetSize().SetCurrentSize(partySize);
        act.GetParty().GetSize().SetMaxSize(partyMax);
    }

    core_->ActivityManager().UpdateActivity(
        act,
        [this](discord::Result res) {
            if (res != discord::Result::Ok) {
                scheduleReconnect(res);
            }
        }
    );
}

void DiscordManager::scheduleReconnect(discord::Result why) {
    // if we're already waiting to reconnect, do nothing
    if (connState_ == ConnectionState::Waiting)
        return;

    core_.reset();
    if (!allowReconnect_) return;

    Logger::getInstance().log(
        "Discord Connection lost (" + std::to_string(static_cast<int>(why)) +
        "); will attempt to reconnect in " + std::to_string(backoffSec_) + "s"
    );

    connState_ = ConnectionState::Waiting;
    nextAttempt_ = std::chrono::steady_clock::now()
        + std::chrono::seconds(backoffSec_);
}

void DiscordManager::finalize() {
    allowReconnect_ = false;
    connState_ = ConnectionState::Disconnected;

    if (core_) {
        core_->ActivityManager().ClearActivity([](discord::Result) {});
        core_->RunCallbacks();
    }

    core_.reset();
}

std::string DiscordManager::getConnectionStateString() const {
    switch (connState_) {
    case ConnectionState::Disconnected: return "Disconnected";
    case ConnectionState::Waiting: return "Waiting";
    case ConnectionState::Connected: return "Connected";
    case ConnectionState::LoadFailed:  return "DLL Load Failed";
    default: return "Unknown";
    }
}

