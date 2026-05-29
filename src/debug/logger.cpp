// logger.cpp
#include "logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>

namespace si {

Logger& Logger::get() {
    static Logger inst;
    return inst;
}

void Logger::configure(LogLevel level, const std::string& path) {
    std::lock_guard<std::mutex> lk(mu_);
    level_ = level;
    if (level == LogLevel::OFF) {
        if (out_.is_open()) out_.close();
        return;
    }
    out_.open(path, std::ios::out | std::ios::trunc);
}

static const char* level_str(LogLevel l) {
    switch (l) {
        case LogLevel::DBG:  return "DEBUG";
        case LogLevel::INFO: return "INFO ";
        case LogLevel::WARN: return "WARN ";
        case LogLevel::ERR:  return "ERROR";
        default:             return "?    ";
    }
}

void Logger::write(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lk(mu_);
    if (!out_.is_open()) return;
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()) % 1000;
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    out_ << std::put_time(&tm, "%H:%M:%S")
         << '.' << std::setw(3) << std::setfill('0') << ms.count()
         << " [" << level_str(level) << "]";
    if (!tag_.empty()) out_ << " [" << tag_ << "]";
    out_ << ' ' << msg << '\n';
    out_.flush();
}

} // namespace si
