#pragma once

/// @file Logger.hpp
/// @brief Thread-safe logging macros that compile to zero overhead in Release builds.
///
/// Usage: LOG_DEBUG("message"), LOG_INFO("message"), LOG_WARN("message"),
///        LOG_ERROR("message"), LOG_FATAL("message")
///
/// FATAL logs the message and calls std::abort().
/// In NDEBUG (Release) builds, ALL macros expand to ((void)0) — zero cost.

#ifdef NDEBUG

// ---- Release: all logging stripped ----
#define LOG_DEBUG(msg) ((void)0)
#define LOG_INFO(msg)  ((void)0)
#define LOG_WARN(msg)  ((void)0)
#define LOG_ERROR(msg) ((void)0)
#define LOG_FATAL(msg) ((void)0)

// Compile-time proof that logging is stripped
namespace LoggerStripped {
    constexpr bool IS_STRIPPED = true;
}

#else

// ---- Debug: full logging ----

#include <iostream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <iomanip>

namespace LoggerStripped {
    constexpr bool IS_STRIPPED = false;
}

namespace LoggerInternal {

enum class Level { Debug, Info, Warn, Error, Fatal };

inline const char* levelStr(Level l) {
    switch (l) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
    }
    return "?????";
}

inline std::mutex& mutex() {
    static std::mutex m;
    return m;
}

inline void log(Level level, const char* file, int line, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    // Extract just the filename, not the full path (security: don't leak directory structure)
    std::string filename(file);
    auto lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    std::ostringstream oss;
    auto tm = *std::localtime(&timeT);
    oss << std::put_time(&tm, "%H:%M:%S") << '.'
        << std::setfill('0') << std::setw(3) << ms.count()
        << " [" << levelStr(level) << "]"
        << "[" << filename << ":" << line << "] "
        << msg;

    std::lock_guard<std::mutex> lock(mutex());
    if (level >= Level::Error) {
        std::cerr << oss.str() << std::endl;
    } else {
        std::cout << oss.str() << std::endl;
    }

    if (level == Level::Fatal) {
        std::abort();
    }
}

} // namespace LoggerInternal

#define LOG_DEBUG(msg) do { \
    std::ostringstream _log_ss; _log_ss << msg; \
    LoggerInternal::log(LoggerInternal::Level::Debug, __FILE__, __LINE__, _log_ss.str()); \
} while(0)

#define LOG_INFO(msg) do { \
    std::ostringstream _log_ss; _log_ss << msg; \
    LoggerInternal::log(LoggerInternal::Level::Info, __FILE__, __LINE__, _log_ss.str()); \
} while(0)

#define LOG_WARN(msg) do { \
    std::ostringstream _log_ss; _log_ss << msg; \
    LoggerInternal::log(LoggerInternal::Level::Warn, __FILE__, __LINE__, _log_ss.str()); \
} while(0)

#define LOG_ERROR(msg) do { \
    std::ostringstream _log_ss; _log_ss << msg; \
    LoggerInternal::log(LoggerInternal::Level::Error, __FILE__, __LINE__, _log_ss.str()); \
} while(0)

#define LOG_FATAL(msg) do { \
    std::ostringstream _log_ss; _log_ss << msg; \
    LoggerInternal::log(LoggerInternal::Level::Fatal, __FILE__, __LINE__, _log_ss.str()); \
} while(0)

#endif // NDEBUG
