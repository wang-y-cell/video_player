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

    /**
     * @brief 获取日志记录器实例
     * @return 日志记录器实例引用
     */
    static Logger& instance();

    /**
    * @brief 设置日志等级
    * @param level 日志等级
    */
    void setLevel(Level level);

    /**
    * @brief 返回日志等级 
    */
    Level level() const;

    /**
    * @brief 判断是否可以打印日志 
    */
    bool shouldLog(Level level) const;

    /**
    * @brief 设置控制台输出 
    * @param enabled 是否启用控制台输出
    */
    void enableConsole(bool enabled);
    /**
    * @brief 设置输入文件
    * @param file_path 日志文件路径
    * @return true 成功
    * @return false 失败
    */
    bool setFile(const std::string& file_path);

    /**
    * @brief 关闭日志文件
    */
    void closeFile();

    /**
    * @brief 记录日志
    * @param level 日志级别
    * @param module 模块名
    * @param message 日志消息
    */ 
    void log(Level level, const char* module, const std::string& message);

    /**
    * @brief 记录FFmpeg日志
    * @param ffmpeg_level FFmpeg日志级别
    * @param message FFmpeg日志消息
    */
    void logFfmpeg(int ffmpeg_level, const std::string& message);

private:
    Logger() = default;

    static const char* levelName(Level level);

    std::atomic<int> level_{static_cast<int>(Level::Info)};
    std::mutex mutex_;
    //是否可以打印日志的条件
    bool console_enabled_ = true;
    std::ofstream file_;
};

#define LOG_DEBUG(module, message_expr)  \
    do {  \
        if (Logger::instance().shouldLog(Logger::Level::Debug)) {                      \
            std::ostringstream _logger_stream;  \
            _logger_stream << message_expr;   \
            Logger::instance().log(Logger::Level::Debug, module, _logger_stream.str());  \
        }   \
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
