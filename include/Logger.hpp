#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>

class Logger {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    static Logger& getInstance();

    void info(const std::string& message, const std::string& component = "");
    void warning(const std::string& message, const std::string& component = "");
    void error(const std::string& message, const std::string& component = "");
    void debug(const std::string& message, const std::string& component = "");

    void setLogLevel(Level level);
    void setLogToFile(bool enable);
    void setLogToConsole(bool enable);
    void setLogFile(const std::string& filename);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    std::ofstream logFile_;
    std::string logFilePath_;
    Level currentLevel_ = Level::INFO;
    bool logToFile_ = false;
    bool logToConsole_ = true;

    std::string getCurrentTime();
    std::string levelToString(Level level);
    void log(Level level, const std::string& message, const std::string& component);
};

#endif