
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

    // read entire file
    std::ifstream ifs(_csvPath, std::ios::binary);
    std::vector<char> buf((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    // undo bit-flip
    flipBuffer(buf);

    // parse lines
    std::istringstream in(std::string(buf.begin(), buf.end()));
    std::string line;
    try {
        // header
        if (!std::getline(in, line) || line != "track,bike,seconds")
            throw std::runtime_error("bad header");

        while (std::getline(in, line)) {
            std::istringstream ss(line);
            std::string track, bike, secStr;
            if (!std::getline(ss, track, ',') ||
                !std::getline(ss, bike, ',') ||
                !std::getline(ss, secStr))
            {
                throw std::runtime_error("malformed line");
            }
            long secs = std::stol(secStr);
            ComboKey k{ track, bike };
            _comboTotals[k] = Seconds(secs);
            _total += Seconds(secs);
        }
    }
    catch (const std::exception& e) {
        Logger::getInstance().log(
            std::string("TimeTracker: corrupted data (") + e.what() +
            "), starting fresh."
        );
        // clear any half-loaded data
        _comboTotals.clear();
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

    auto elapsed = std::chrono::duration_cast<Seconds>(Clock::now() - _runStart);
    _comboTotals[_activeKey] += elapsed;
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
    str = str.substr(0, str.size() - 4);

    return str;
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

    // build data file
    std::ostringstream out;
    out << "track,bike,seconds\n";
    for (auto const& [key, sec] : _comboTotals) {
        out << key.track << "," << key.bike << "," << sec.count() << "\n";
    }
    std::string txt = out.str();

    // bit-flip
    std::vector<char> buf(txt.begin(), txt.end());
    flipBuffer(buf);

    // atomically write
    auto tmp = _csvPath.string() + ".tmp";
    {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs.write(buf.data(), buf.size());
    }
    std::filesystem::rename(tmp, _csvPath);

    Logger::getInstance().log("Stats updated");
}
