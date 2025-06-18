
// timeTracker.h

#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <ctime>               // for std::time_t

class TimeTracker {
public:
    static TimeTracker& getInstance();

    // Now takes the full file path...
    void initialize(const std::filesystem::path& csvPath);

    void startRun(const std::string& trackID, const std::string& bikeID);
    void endRun(const std::string& trackID, const std::string& bikeID);

    std::string getCurrentComboTime() const;
    std::string getTotalTime()        const;

    void save() const;

private:
    TimeTracker() = default;
    ~TimeTracker() = default;
    TimeTracker(const TimeTracker&) = delete;
    TimeTracker& operator=(const TimeTracker&) = delete;

    using Clock = std::chrono::steady_clock;
    using Seconds = std::chrono::seconds;

    struct ComboKey {
        std::string track, bike;
        bool operator==(ComboKey const& o) const {
            return track == o.track && bike == o.bike;
        }
    };
    struct ComboKeyHash {
        size_t operator()(ComboKey const& k) const noexcept {
            return std::hash<std::string>()(k.track) * 31
                ^ std::hash<std::string>()(k.bike);
        }
    };

    void load();

    std::filesystem::path                                   _csvPath;
    mutable std::mutex                                      _mtx;
    ComboKey                                                _activeKey;
    Clock::time_point                                       _runStart;
    bool                                                    _isRunning{ false };
    std::unordered_map<ComboKey, Seconds, ComboKeyHash>     _comboTotals;
    Seconds                                                 _total{ 0 };

    // NEW: track first and last run timestamps (epoch seconds)
    std::unordered_map<ComboKey, std::time_t, ComboKeyHash> _firstRun;
    std::unordered_map<ComboKey, std::time_t, ComboKeyHash> _lastRun;
};
