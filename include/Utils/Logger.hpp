#pragma once

#include <atomic>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

class Logger {
public:
    enum class Level {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Off = 4
    };

    static Logger& instance();

    void setLevel(Level level);
    Level level() const;
    bool shouldLog(Level level) const;

    void enableConsole(bool enabled);
    bool setFile(const std::string& file_path);
    void closeFile();

    void log(Level level, const char* module, const std::string& message);
    void logFfmpeg(int ffmpeg_level, const std::string& message);

private:
    Logger() = default;

    static const char* levelName(Level level);

    std::atomic<int> level_{static_cast<int>(Level::Info)};
    std::mutex mutex_;
    bool console_enabled_ = true;
    std::ofstream file_;
};

#define LOG_DEBUG(module, message_expr)                                                                                  \
    do {                                                                                                                 \
        if (Logger::instance().shouldLog(Logger::Level::Debug)) {                                                        \
            std::ostringstream _logger_stream;                                                                           \
            _logger_stream << message_expr;                                                                              \
            Logger::instance().log(Logger::Level::Debug, module, _logger_stream.str());                                 \
        }                                                                                                                \
    } while (0)

#define LOG_INFO(module, message_expr)                                                                                   \
    do {                                                                                                                 \
        if (Logger::instance().shouldLog(Logger::Level::Info)) {                                                         \
            std::ostringstream _logger_stream;                                                                           \
            _logger_stream << message_expr;                                                                              \
            Logger::instance().log(Logger::Level::Info, module, _logger_stream.str());                                  \
        }                                                                                                                \
    } while (0)

#define LOG_WARN(module, message_expr)                                                                                   \
    do {                                                                                                                 \
        if (Logger::instance().shouldLog(Logger::Level::Warn)) {                                                         \
            std::ostringstream _logger_stream;                                                                           \
            _logger_stream << message_expr;                                                                              \
            Logger::instance().log(Logger::Level::Warn, module, _logger_stream.str());                                  \
        }                                                                                                                \
    } while (0)

#define LOG_ERROR(module, message_expr)                                                                                  \
    do {                                                                                                                 \
        if (Logger::instance().shouldLog(Logger::Level::Error)) {                                                        \
            std::ostringstream _logger_stream;                                                                           \
            _logger_stream << message_expr;                                                                              \
            Logger::instance().log(Logger::Level::Error, module, _logger_stream.str());                                 \
        }                                                                                                                \
    } while (0)
