
// timeTracker.cpp

#include "pch.h"

#include "Constants.h"
#include "timeTracker.h"
#include "Logger.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <filesystem>
#include <ctime>
#include <limits>
#include <algorithm>

using Seconds = std::chrono::seconds;
using Rep = Seconds::rep;
using Clock = std::chrono::steady_clock;

// simple bit-flip
static void flipBuffer(std::vector<char>& buf) {
    for (auto& b : buf) {
        b = ~b;
    }
}

TimeTracker& TimeTracker::getInstance() {
    static TimeTracker inst;
    return inst;
}

void TimeTracker::initialize(const std::filesystem::path& datPath) {
    std::lock_guard lk(_mtx);
    _datPath = datPath;
    load();
    Logger::getInstance().log("TimeTracker initialized with file: " + _datPath.string());
}

void TimeTracker::load() {
    if (!std::filesystem::exists(_datPath)) return;

    // Read and flip
    std::ifstream ifs(_datPath, std::ios::binary);
    std::vector<char> buf((std::istreambuf_iterator<char>(ifs)), {});
    ifs.close();
    flipBuffer(buf);
    std::istringstream in(std::string(buf.begin(), buf.end()));
    std::string line;

    if (!std::getline(in, line)) return;
    std::vector<std::string> headers;
    {
        std::istringstream hs(line);
        std::string h;
        while (std::getline(hs, h, ',')) headers.push_back(h);
    }
    auto idx = [&](std::string name) -> int {
        auto it = std::find(headers.begin(), headers.end(), name);
        return it == headers.end() ? -1 : int(std::distance(headers.begin(), it));
        };

    // Clear existing state
    _comboTotals.clear();
    _firstRun.clear();
    _lastRun.clear();
    _alltimeBestLapMs.clear();
    _alltimeBestLapTs.clear();
    _alltimeLapCount.clear();
    _bikeCategory.clear();
    _alltimeBestLapSplitsMs.clear();
    _alltimeBestLapSetup.clear();
    _total = Seconds{ 0 };

    // Must at least have track & bike columns
    int track = idx("track"), bike = idx("bike");
    if (track < 0 || bike < 0) {
        Logger::getInstance().log("TimeTracker: missing track/bike columns, skipping load.");
        return;
    }

    // Other optional columns
    int firstrun_ts = idx("firstrun_ts"); if (firstrun_ts < 0) firstrun_ts = idx("firstrun"); // backwards compatibility
    int lastrun_ts = idx("lastrun_ts");  if (lastrun_ts < 0) lastrun_ts = idx("lastrun");   // backwards compatibility
    int tracktime_s = idx("tracktime_s"); if (tracktime_s < 0) tracktime_s = idx("seconds");   // backwards compatibility
    int bestlap_ms = idx("bestlap_ms");
    int bestlap_ts = idx("bestlap_ts");
    int numlaps = idx("numlaps");
    int category = idx("category");
    int split1_ms = idx("split1_ms");
    int split2_ms = idx("split2_ms");
    int split3_ms = idx("split3_ms");
    int bestlap_setup = idx("bestlap_setup");

    // Process each line, skipping any bad ones
    std::string line2;
    while (std::getline(in, line2)) {
        try {
            std::vector<std::string> tok;
            std::istringstream ss(line2);
            std::string cell;
            while (std::getline(ss, cell, ',')) tok.push_back(cell);

            // require at least track & bike
            if (int(tok.size()) <= std::max<int>(track, bike))
                throw std::runtime_error("not enough columns");

            ComboKey key{ tok[track], tok[bike] };

            if (category >= 0 && int(tok.size()) > category) {
                _bikeCategory[key] = tok[category];
            }

            // first/last run timestamps
            if (firstrun_ts >= 0 && int(tok.size()) > firstrun_ts) {
                _firstRun[key] = std::stoll(tok[firstrun_ts]);
            }
            if (lastrun_ts >= 0 && int(tok.size()) > lastrun_ts) {
                _lastRun[key] = std::stoll(tok[lastrun_ts]);
            }

            // total time
            long s = (tracktime_s >= 0 && int(tok.size()) > tracktime_s) ? std::stol(tok[tracktime_s]) : 0;
            _comboTotals[key] = Seconds(s);
            _total += Seconds(s);

            // all-time PB
            if (bestlap_ts >= 0 && bestlap_ms >= 0
                && int(tok.size()) > bestlap_ts
                && int(tok.size()) > bestlap_ms)
            {
                int bestMs = std::stoi(tok[bestlap_ms]);
                if (bestMs > 0) {
                    _alltimeBestLapTs[key] = std::stoll(tok[bestlap_ts]);
                    _alltimeBestLapMs[key] = bestMs;

                    if (bestlap_setup >= 0 && int(tok.size()) > bestlap_setup) {
                        _alltimeBestLapSetup[key] = tok[bestlap_setup];
                    }
                }
            }

            // Best lap split segments
            if (_alltimeBestLapMs.count(key)) {
                std::array<int, 3> segs{ 0,0,0 };
                if (split1_ms >= 0 && int(tok.size()) > split1_ms) segs[0] = std::stoi(tok[split1_ms]);
                if (split2_ms >= 0 && int(tok.size()) > split2_ms) segs[1] = std::stoi(tok[split2_ms]);
                if (split3_ms >= 0 && int(tok.size()) > split3_ms) segs[2] = std::stoi(tok[split3_ms]);
                _alltimeBestLapSplitsMs[key] = segs;
            }

            // laps
            if (numlaps >= 0 && int(tok.size()) > numlaps) {
                _alltimeLapCount[key] = std::stoi(tok[numlaps]);
            }
            else {
                _alltimeLapCount[key] = 0;
            }
        }
        catch (const std::exception& e) {
            Logger::getInstance().log(
                "TimeTracker: skipped bad line: \"" + line2 + "\" (" + e.what() + ")");
            continue;
        }
    }
}

void TimeTracker::startRun(const std::string & trackID, const std::string & bikeID, const std::string & bikeCategory, const std::string & setupName) {
    std::lock_guard lk(_mtx);
    _activeKey = ComboKey{ trackID, bikeID };
    _runStart = Clock::now();
    _isRunning = true;
    _sessionLapCount = 0;
    _bikeCategory[_activeKey] = bikeCategory;
    _activeSetupName = setupName;
}

void TimeTracker::endRun(const std::string& trackID, const std::string& bikeID) {
    std::lock_guard lk(_mtx);
    if (!_isRunning ||
        _activeKey.track != trackID ||
        _activeKey.bike != bikeID)
        return;

    auto nowSys = std::chrono::system_clock::now();
    auto nowEpoch = std::chrono::system_clock::to_time_t(nowSys);
    auto elapsed = std::chrono::duration_cast<Seconds>(Clock::now() - _runStart);

    auto& totalSecs = _comboTotals[_activeKey];
    // record first run if not already set
    if (_firstRun.find(_activeKey) == _firstRun.end() || _firstRun[_activeKey] == 0) {
        _firstRun[_activeKey] = nowEpoch;
    }
    totalSecs += elapsed;
    _lastRun[_activeKey] = nowEpoch;

    _total += elapsed;
    _isRunning = false;
}

void TimeTracker::resetSessionPB() {
    std::lock_guard lk(_mtx);
    _sessionBestLapMs = (std::numeric_limits<int>::max)();
    _sessionLapCount = 0;
}

// Helper to format seconds as "HHh MMm SSs"
static std::string fmtHMS(Rep seconds) {
    Rep h = seconds / 3600;
    Rep m = (seconds % 3600) / 60;
    Rep s = seconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << h << "h "
        << std::setw(2) << m << "m "
        << std::setw(2) << s << "s";

    std::string str = oss.str();
    // No one cares about seconds
    return str.substr(0, str.size() - 4);
}

// Helper to format milliseconds as "M:SS.mmm"
static std::string formatMs(int milliseconds) {
    int m = milliseconds / 60000;
    int s = (milliseconds % 60000) / 1000;
    int ms = milliseconds % 1000;
    std::ostringstream oss;
    oss
        << m << ':'
        << std::setw(2) << std::setfill('0') << s << '.'
        << std::setw(3) << std::setfill('0') << ms;
    return oss.str();
}

std::string TimeTracker::getComboTime() const {
    std::lock_guard lk(_mtx);
    Rep base = 0;
    if (auto it = _comboTotals.find(_activeKey); it != _comboTotals.end())
        base = it->second.count();
    if (_isRunning)
        base += std::chrono::duration_cast<Seconds>(Clock::now() - _runStart).count();
    return fmtHMS(base);
}

std::string TimeTracker::getTotalTime() const {
    std::lock_guard lk(_mtx);
    Rep secs = _total.count();
    if (_isRunning)
        secs += std::chrono::duration_cast<Seconds>(Clock::now() - _runStart).count();
    return fmtHMS(secs);
}

void TimeTracker::recordLap(const std::string& trackID, const std::string& bikeID, int lapTimeMs, const std::vector<int>& cumulativeSplitsMs) {
    std::lock_guard lk(_mtx);
    ComboKey key{ trackID, bikeID };
    if (!_isRunning || key != _activeKey) return;

    // Session PB
    if (_sessionBestLapMs == (std::numeric_limits<int>::max)() || lapTimeMs < _sessionBestLapMs) {
        _sessionBestLapMs = lapTimeMs;
    }

    // Derive 3 segment times
    std::array<int, 3> segs{ 0,0,0 };
    if (!cumulativeSplitsMs.empty()) {
        int s1 = cumulativeSplitsMs.size() > 0 ? cumulativeSplitsMs[0] : 0;
        int s2 = cumulativeSplitsMs.size() > 1 ? cumulativeSplitsMs[1] : 0;

        // Clamp to sane ranges to avoid negatives if anything odd slips through
        s1 = std::max<int>(0, std::min<int>(s1, lapTimeMs));
        s2 = std::max<int>(s1, std::min<int>(s2, lapTimeMs));

        segs[0] = s1;
        segs[1] = s2 - s1;
        segs[2] = lapTimeMs - s2;
        // Final guard
        if (segs[2] < 0) segs[2] = 0;
    }
    else {
        // No split callbacks received; whole lap is one segment (last)
        segs = { 0, 0, lapTimeMs };
    }

    // All-time PB
    auto it = _alltimeBestLapMs.find(key);
    std::time_t nowEpoch = std::time(nullptr);
    if (it == _alltimeBestLapMs.end() || lapTimeMs < it->second) {
        _alltimeBestLapTs[key] = nowEpoch;
        _alltimeBestLapMs[key] = lapTimeMs;
        _alltimeBestLapSplitsMs[key] = segs;
        _alltimeBestLapSetup[key] = _activeSetupName;
    }

    // Laps
    _alltimeLapCount[key] += 1;
    _sessionLapCount += 1;
}

std::string TimeTracker::getSessionPB() const {
    std::lock_guard lk(_mtx);
    if (_sessionBestLapMs == (std::numeric_limits<int>::max)()) return "0:00.000";
    return formatMs(_sessionBestLapMs);
}

std::string TimeTracker::getAlltimePB() const {
    std::lock_guard lk(_mtx);
    auto it = _alltimeBestLapMs.find(_activeKey);
    if (it == _alltimeBestLapMs.end()) return "0:00.000";
    return formatMs(it->second);
}

std::string TimeTracker::getComboLapCount() const {
    std::lock_guard lk(_mtx);
    auto it = _alltimeLapCount.find(_activeKey);
    int laps = (it == _alltimeLapCount.end()) ? 0 : it->second;
    return std::to_string(laps);
}

std::string TimeTracker::getTotalLapCount() const {
    std::lock_guard lk(_mtx);
    int sum = 0;
    for (const auto& kv : _alltimeLapCount) sum += kv.second;
    return std::to_string(sum);
}

void TimeTracker::save() const {
    std::lock_guard lk(_mtx);

    // Build content
    std::ostringstream out;
    out << "track,bike,category,tracktime_s,numlaps,firstrun_ts,lastrun_ts,bestlap_ts,bestlap_ms,split1_ms,split2_ms,split3_ms,bestlap_setup\n";

    for (auto const& [key, tracktime_s] : _comboTotals) {
        std::time_t firstrun_ts = (_firstRun.count(key) ? _firstRun.at(key) : 0);
        std::time_t lastrun_ts = (_lastRun.count(key) ? _lastRun.at(key) : 0);
        std::string category = _bikeCategory.count(key) ? _bikeCategory.at(key) : "";

        std::time_t bestlap_ts = _alltimeBestLapTs.count(key) ? _alltimeBestLapTs.at(key) : 0;
        int bestlap_ms = _alltimeBestLapMs.count(key) ? _alltimeBestLapMs.at(key) : 0;
        int numlaps = _alltimeLapCount.count(key) ? _alltimeLapCount.at(key) : 0;

        std::array<int, 3> segs{ 0,0,0 };
        if (_alltimeBestLapSplitsMs.count(key)) segs = _alltimeBestLapSplitsMs.at(key);
        std::string bestlap_setup = _alltimeBestLapSetup.count(key) ? _alltimeBestLapSetup.at(key) : "";

        out
            << key.track << ","
            << key.bike << ","
            << category << ","
            << tracktime_s.count() << ","
            << numlaps << ","
            << firstrun_ts << ","
            << lastrun_ts << ","
            << bestlap_ts << ","
            << bestlap_ms << ","
            << segs[0] << ","
            << segs[1] << ","
            << segs[2] << ","
            << bestlap_setup
            << "\n";
    }

    std::string txt = out.str();

    // Write .csv (human-readable companion)
    std::filesystem::path csvPath = _datPath;
    csvPath.replace_filename(csvPath.stem().string() + "-times.csv");
    {
        std::ofstream ofsPlain(csvPath, std::ios::trunc);
        ofsPlain << txt;
    }

    // Write .dat
    std::vector<char> buf(txt.begin(), txt.end());
    flipBuffer(buf);

    auto tmp = _datPath.string() + ".tmp";
    {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs.write(buf.data(), buf.size());
    }
    std::filesystem::rename(tmp, _datPath);

    Logger::getInstance().log("Stats updated");
}
