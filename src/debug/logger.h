// logger.h - leveled, thread-safe logging to file.
//
// Used by net/ai/replay modules to record events that would otherwise be
// invisible, such as handshake bytes or AI decisions. This is especially
// useful when debugging lockstep networking.
//
// Usage:
//     LOG_INFO("server listening on " << port);
//     LOG_DEBUG("ai chose LEFT (utility = " << u << ")");
//
// Log is buffered through std::ofstream and protected by a mutex.
// File path defaults to "si_pro.log" in the working directory.
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

    // Configure once at startup. Default level is INFO; default file is
    // si_pro.log. Pass LogLevel::OFF to suppress logging entirely.
    void configure(LogLevel level, const std::string& path);

    bool enabled(LogLevel level) const { return level >= level_; }
    void write(LogLevel level, const std::string& msg);

    // Set process-wide tag (e.g. "HOST" / "CLIENT") prepended to lines.
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

} // namespace si

// windows.h defines ERROR and (in some configurations) DEBUG as macros,
// which would silently corrupt our LogLevel enum. We can't always control
// header include order in client code, so undef them here defensively.
// Undefining them here keeps this header usable even when a platform
// header was included first.
#ifdef ERROR
#  undef ERROR
#endif
#ifdef DEBUG
#  undef DEBUG
#endif

// Macros that compile away cleanly when the level is too low.
#define SI_LOG(LVL, EXPR) do { \
    auto& _lg = ::si::Logger::get(); \
    if (_lg.enabled(LVL)) { std::ostringstream _ss; _ss << EXPR; _lg.write(LVL, _ss.str()); } \
} while (0)

#define LOG_DEBUG(EXPR) SI_LOG(::si::LogLevel::DBG,  EXPR)
#define LOG_INFO(EXPR)  SI_LOG(::si::LogLevel::INFO, EXPR)
#define LOG_WARN(EXPR)  SI_LOG(::si::LogLevel::WARN, EXPR)
#define LOG_ERROR(EXPR) SI_LOG(::si::LogLevel::ERR,  EXPR)
