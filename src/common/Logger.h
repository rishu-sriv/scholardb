#pragma once

// Logger.h — lightweight thread-safe logger with timestamps.
// Header-only. The static mutex is declared inline (C++17).
//
// Usage:
//   Logger::info ("server", "client connected id=42");
//   Logger::error("server", "Student not found");

#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

class Logger {
public:
    static void info (const std::string& tag, const std::string& msg) { log("INFO ", tag, msg, false); }
    static void warn (const std::string& tag, const std::string& msg) { log("WARN ", tag, msg, false); }
    static void error(const std::string& tag, const std::string& msg) { log("ERROR", tag, msg, true);  }

private:
    static void log(const std::string& level,
                    const std::string& tag,
                    const std::string& msg,
                    bool               toStderr)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now  = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%H:%M:%S")
            << " [" << level << "] [" << tag << "] " << msg;

        (toStderr ? std::cerr : std::cout) << oss.str() << "\n";
    }

    static inline std::mutex mutex_;
};
