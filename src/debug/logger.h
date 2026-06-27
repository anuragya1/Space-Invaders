#pragma once

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace si {

enum class LogLevel { DBG = 0, INFO = 1, WARN = 2, ERR = 3, OFF = 4 };

class Logger {
public:
    static Logger& get();

    void configure(LogLevel level, const std::string& path);

    bool enabled(LogLevel level) const { return level >= level_; }
    void write(LogLevel level, const std::string& msg);

    void set_tag(std::string tag) { tag_ = std::move(tag); }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex     mu_;
    std::ofstream  out_;
    LogLevel       level_ = LogLevel::OFF;
    std::string    tag_;
};

}

#ifdef ERROR
#  undef ERROR
#endif
#ifdef DEBUG
#  undef DEBUG
#endif

#define SI_LOG(LVL, EXPR) do { \
    auto& _lg = ::si::Logger::get(); \
    if (_lg.enabled(LVL)) { std::ostringstream _ss; _ss << EXPR; _lg.write(LVL, _ss.str()); } \
} while (0)

#define LOG_DEBUG(EXPR) SI_LOG(::si::LogLevel::DBG,  EXPR)
#define LOG_INFO(EXPR)  SI_LOG(::si::LogLevel::INFO, EXPR)
#define LOG_WARN(EXPR)  SI_LOG(::si::LogLevel::WARN, EXPR)
#define LOG_ERROR(EXPR) SI_LOG(::si::LogLevel::ERR,  EXPR)
