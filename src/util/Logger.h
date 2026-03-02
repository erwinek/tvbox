#pragma once

#include <string>

namespace util {

enum class LogLevel {
    Info,
    Warn,
    Error
};

void Log(LogLevel level, const std::string& message);

}  // namespace util
