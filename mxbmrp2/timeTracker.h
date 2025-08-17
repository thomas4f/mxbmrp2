
// timeTracker.h

#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <ctime>
#include <limits>
#include <array>
#include <vector>

class TimeTracker {
public:
    static TimeTracker& getInstance();

    void initialize(const std::filesystem::path& csvPath);
    void startRun(const std::string & trackID, const std::string & bikeID, const std::string & bikeCategory, const std::string & setupName);
    void recordLap(const std::string& trackID, const std::string& bikeID, int lapTimeMs, const std::vector<int>& cumulativeSplitsMs);
    void endRun(const std::string& trackID, const std::string& bikeID);

    std::string getComboTime() const;
    std::string getTotalTime() const;
    std::string getSessionPB() const;
    std::string getAlltimePB() const;
    std::string getComboLapCount() const;
    std::string getTotalLapCount() const;
    void resetSessionPB();

    void save() const;

private:
    TimeTracker() = default;
    ~TimeTracker() = default;
    TimeTracker(const TimeTracker&) = delete;
    TimeTracker& operator=(const TimeTracker&) = delete;

    using Clock = std::chrono::steady_clock;
    using Seconds = std::chrono::seconds;

    struct ComboKey {
        std::string track;
        std::string bike;

        bool operator==(ComboKey const& rhs) const {
            return track == rhs.track
                && bike == rhs.bike;
        }
        bool operator!=(ComboKey const& rhs) const {
            return !(*this == rhs);
        }
    };

    struct ComboKeyHash {
        size_t operator()(ComboKey const& k) const noexcept {
            return std::hash<std::string>()(k.track) * 31
                ^ std::hash<std::string>()(k.bike);
        }
    };

    void load();

    std::filesystem::path _datPath;
    mutable std::mutex _mtx;
    ComboKey _activeKey;
    Clock::time_point _runStart;
    bool _isRunning{ false };
    std::string _activeSetupName;
    std::unordered_map<ComboKey, Seconds, ComboKeyHash> _comboTotals;
    Seconds _total{ 0 };

    std::unordered_map<ComboKey, std::time_t, ComboKeyHash> _firstRun;
    std::unordered_map<ComboKey, std::time_t, ComboKeyHash> _lastRun;

    int _sessionLapCount = 0;
    std::unordered_map<ComboKey, int, ComboKeyHash> _alltimeBestLapMs;
    std::unordered_map<ComboKey, std::time_t, ComboKeyHash> _alltimeBestLapTs;
    std::unordered_map<ComboKey, std::array<int, 3>, ComboKeyHash> _alltimeBestLapSplitsMs;
    int _sessionBestLapMs = (std::numeric_limits<int>::max)();
    std::unordered_map<ComboKey, int, ComboKeyHash> _alltimeLapCount;
    std::unordered_map<ComboKey, std::string, ComboKeyHash> _bikeCategory;
    std::unordered_map<ComboKey, std::string, ComboKeyHash> _alltimeBestLapSetup;
};
