#include "Logger.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <thread>

extern "C" {
#include <libavutil/log.h>
}

namespace {
std::string trimMessage(const std::string& input) {
    std::string result = input;
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

std::string buildTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto time = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_value{};
    localtime_r(&time, &tm_value);

    std::ostringstream stream;
    stream << std::put_time(&tm_value, "%Y-%m-%d %H:%M:%S") << '.'
           << std::setw(3) << std::setfill('0') << ms.count();
    return stream.str();
}

std::string buildThreadId() {
    std::ostringstream stream;
    stream << std::this_thread::get_id();
    return stream.str();
}
} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLevel(Level level) {
    level_.store(static_cast<int>(level));
}

Logger::Level Logger::level() const {
    return static_cast<Level>(level_.load());
}

bool Logger::shouldLog(Level level) const {
    const int current_level = level_.load();
    return current_level != static_cast<int>(Level::Off) && static_cast<int>(level) >= current_level;
}

void Logger::enableConsole(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_enabled_ = enabled;
}

bool Logger::setFile(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::filesystem::path path(file_path);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    file_.close();
    file_.clear();
    file_.open(file_path, std::ios::out | std::ios::app);
    return file_.is_open();
}

void Logger::closeFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::log(Level level, const char* module, const std::string& message) {
    if (!shouldLog(level)) {
        return;
    }

    const std::string text = trimMessage(message);
    if (text.empty()) {
        return;
    }

    std::ostringstream stream;
    stream << buildTimestamp() << " [" << levelName(level) << "]"
           << " [" << (module ? module : "Unknown") << "]"
           << " [tid:" << buildThreadId() << "] "
           << text;
    const std::string line = stream.str();

    std::lock_guard<std::mutex> lock(mutex_);
    if (console_enabled_) {
        std::cerr << line << '\n';
    }
    if (file_.is_open()) {
        file_ << line << '\n';
        file_.flush();
    }
}

void Logger::logFfmpeg(int ffmpeg_level, const std::string& message) {
    Level mapped_level = Level::Debug;
    if (ffmpeg_level <= AV_LOG_ERROR) {
        mapped_level = Level::Error;
    } else if (ffmpeg_level <= AV_LOG_WARNING) {
        mapped_level = Level::Warn;
    } else if (ffmpeg_level <= AV_LOG_INFO) {
        mapped_level = Level::Info;
    }
    log(mapped_level, "FFmpeg", message);
}

const char* Logger::levelName(Level level) {
    switch (level) {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        case Level::Off:
            return "OFF";
    }
    return "INFO";
}
