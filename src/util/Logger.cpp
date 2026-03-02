#include "util/Logger.h"

#include <chrono>
#include <ctime>
#include <iostream>

namespace util {

static const char* LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "INFO";
    }
}

void Log(LogLevel level, const std::string& message) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_local{};
#ifdef _WIN32
    localtime_s(&tm_local, &now_t);
#else
    localtime_r(&now_t, &tm_local);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_local);
    std::cout << "[" << buf << "] " << LevelToString(level) << " " << message << std::endl;
}

}  // namespace util
