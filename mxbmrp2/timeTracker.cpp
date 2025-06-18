
// timeTracker.cpp

#include "pch.h"
#include "timeTracker.h"
#include "Logger.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <filesystem>
#include <system_error>
#include <ctime>

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

void TimeTracker::initialize(const std::filesystem::path& csvPath) {
    std::lock_guard lk(_mtx);
    _csvPath = csvPath;
    load();
    Logger::getInstance().log("TimeTracker initialized with file: " + _csvPath.string());
}

void TimeTracker::load() {
    if (!std::filesystem::exists(_csvPath)) return;

    std::ifstream ifs(_csvPath, std::ios::binary);
    std::vector<char> buf((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    flipBuffer(buf);
    std::istringstream in(std::string(buf.begin(), buf.end()));
    std::string line;
    try {
        if (!std::getline(in, line) || line != "track,bike,firstrun,lastrun,seconds")
            throw std::runtime_error("bad header");

        while (std::getline(in, line)) {
            std::istringstream ss(line);
            std::string track, bike, firstStr, lastStr, secStr;
            if (!std::getline(ss, track, ',') ||
                !std::getline(ss, bike, ',') ||
                !std::getline(ss, firstStr, ',') ||
                !std::getline(ss, lastStr, ',') ||
                !std::getline(ss, secStr))
            {
                throw std::runtime_error("malformed line");
            }

            ComboKey k{ track, bike };
            long secs = std::stol(secStr);
            _comboTotals[k] = Seconds(secs);
            _total += Seconds(secs);

            // parse and store first/last run
            _firstRun[k] = static_cast<std::time_t>(std::stoll(firstStr));
            _lastRun[k] = static_cast<std::time_t>(std::stoll(lastStr));
        }
    }
    catch (const std::exception& e) {
        Logger::getInstance().log(
            std::string("TimeTracker: corrupted data (") + e.what() +
            "), starting fresh."
        );
        _comboTotals.clear();
        _firstRun.clear();
        _lastRun.clear();
        _total = Seconds{ 0 };

        std::error_code ec;
        std::filesystem::remove(_csvPath, ec);
    }
}

void TimeTracker::startRun(const std::string& trackID, const std::string& bikeID) {
    std::lock_guard lk(_mtx);
    _activeKey = ComboKey{ trackID, bikeID };
    _runStart = Clock::now();
    _isRunning = true;
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

// format as "HHh MMm SSs"
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

std::string TimeTracker::getCurrentComboTime() const {
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

void TimeTracker::save() const {
    std::lock_guard lk(_mtx);

    // 1) Build content
    std::ostringstream out;
    out << "track,bike,firstrun,lastrun,seconds\n";
    for (auto const& [key, totalSec] : _comboTotals) {
        std::time_t first = (_firstRun.count(key) ? _firstRun.at(key) : 0);
        std::time_t last = (_lastRun.count(key) ? _lastRun.at(key) : 0);
        out
            << key.track << ","
            << key.bike << ","
            << first << ","
            << last << ","
            << totalSec.count()
            << "\n";
    }
    std::string txt = out.str();

    // 2) Write .csv
    {
        auto plainPath = _csvPath.parent_path() / "mxbmrp2-times.csv";
        std::ofstream ofsPlain(plainPath, std::ios::trunc);
        ofsPlain << txt;
    }

    // 3) Write .dat file
    std::vector<char> buf(txt.begin(), txt.end());
    flipBuffer(buf);

    auto tmp = _csvPath.string() + ".tmp";
    {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs.write(buf.data(), buf.size());
    }
    std::filesystem::rename(tmp, _csvPath);

    Logger::getInstance().log("Stats updated");
}
