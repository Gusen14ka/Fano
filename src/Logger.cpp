#include "Logger.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

Logger::Logger() = default;

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::levelToString(Level level) {
    switch (level) {
        case Level::INFO: return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR: return "ERROR";
        case Level::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

void Logger::log(Level level, const std::string& message, const std::string& component) {
    if (level < currentLevel_) {
        return;
    }

    std::string logEntry = "[" + getCurrentTime() + "] [" + levelToString(level) + "]";
    if (!component.empty()) {
        logEntry += " [" + component + "]";
    }
    logEntry += " " + message;

    if (logToConsole_) {
        if (level == Level::ERROR) {
            std::cerr << logEntry << std::endl;
        } else {
            std::cout << logEntry << std::endl;
        }
    }

    if (logToFile_ && logFile_.is_open()) {
        logFile_ << logEntry << std::endl;
    }
}

void Logger::info(const std::string& message, const std::string& component) {
    log(Level::INFO, message, component);
}

void Logger::warning(const std::string& message, const std::string& component) {
    log(Level::WARNING, message, component);
}

void Logger::error(const std::string& message, const std::string& component) {
    log(Level::ERROR, message, component);
}

void Logger::debug(const std::string& message, const std::string& component) {
    log(Level::DEBUG, message, component);
}

void Logger::setLogLevel(Level level) {
    currentLevel_ = level;
}

void Logger::setLogToFile(bool enable) {
    logToFile_ = enable;
    if (enable && !logFile_.is_open() && !logFilePath_.empty()) {
        logFile_.open(logFilePath_, std::ios::app);
    }
}

void Logger::setLogToConsole(bool enable) {
    logToConsole_ = enable;
}

void Logger::setLogFile(const std::string& filename) {
    logFilePath_ = filename;
    if (logFile_.is_open()) {
        logFile_.close();
    }
    if (logToFile_) {
        logFile_.open(logFilePath_, std::ios::app);
    }
}