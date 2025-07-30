// Logger.cpp

#include "pch.h"

#include <mutex>
#include <iomanip>
#ifdef _DEBUG
#include <iostream>
#endif

#include "Logger.h"

// Singleton instance
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// Constructor
Logger::Logger() = default;

// Destructor
Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

// Opens the log file
void Logger::openLogFile() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFile_.open(logFilePath_, std::ios::out | std::ios::trunc);
    if (!logFile_) {
        // Handle the error as needed
    }
}

// Logs a message to the log file
void Logger::log(const std::string& message, bool newline) {
    // get current time
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);

    std::lock_guard<std::mutex> lk(mutex_);

#ifdef _DEBUG
    std::cout
        << std::put_time(&tm, "[%H:%M:%S] ")
        << message
        << (newline ? "\n" : "");
#endif

    if (!logFile_.is_open()) return;
    logFile_
        << std::put_time(&tm, "[%H:%M:%S] ")
        << message;
    if (newline) logFile_ << std::endl;
}

// Sets the log file name
void Logger::setLogFileName(const std::filesystem::path& filePath) {
    std::lock_guard<std::mutex> lk(mutex_);
    logFilePath_ = filePath;
    openLogFile();
}
