
// Logger.cpp

#include "pch.h"
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
    std::lock_guard<std::mutex> guard(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

// Opens the log file
void Logger::openLogFile() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFile_.open(logFileName_, std::ios::out | std::ios::trunc);
    if (!logFile_) {
        // Handle the error as needed
    }
}

// Logs a message to the log file
void Logger::log(const std::string& message, bool newline) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (logFile_.is_open()) {
        logFile_ << message;
        if (newline) {
            logFile_ << std::endl;
        }
    }
}

// Clears the log file
void Logger::truncate() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFile_.open(logFileName_, std::ios::out | std::ios::trunc);
    if (!logFile_) {
        // Handle the error as needed
    }
}

// Sets the log file name
void Logger::setLogFileName(const std::string& fileName) {
    std::lock_guard<std::mutex> guard(mutex_);
    logFileName_ = fileName;
    openLogFile();
}
