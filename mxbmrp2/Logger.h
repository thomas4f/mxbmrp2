
// Logger.h

#pragma once

#include <fstream>
#include <mutex>

class Logger {
public:
    // Retrieves the singleton instance of Logger
    static Logger& getInstance();

    // Logs a message to the log file
    void log(const std::string& message, bool newline = true);

    // Clear the log file
    void truncate();

    // Set the log file name (must be called before any logging)
    void setLogFileName(const std::string& fileName);

private:
    // Private constructor to enforce singleton pattern
    Logger();

    // Private destructor to ensure the log file is closed when the Logger is destroyed
    ~Logger();

    // Delete copy constructor and assignment operator to prevent copies
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string logFileName_;
    std::mutex mutex_;
    std::ofstream logFile_;

    // Internal helper to open the log file
    void openLogFile();
};