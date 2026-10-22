#pragma once

#include "common.h"
#include <string>

namespace myspdlog {

// 日志级别
enum class Level {
    LOG_TRACE = 0,      // 详细的调试信息
    LOG_DEBUG = 1,      // 调试信息
    LOG_INFO = 2,       // 普通信息
    LOG_WARN = 3,       // 警告信息
    LOG_ERROR = 4,      // 错误信息
    LOG_CRITICAL = 5,   // 严重错误
    LOG_OFF = 6         // 关闭日志
};

MYSPDLOG_API const char* level_to_string(Level lvl) noexcept;
MYSPDLOG_API const char* level_to_short_string(Level lvl) noexcept;
MYSPDLOG_API Level string_to_level(const std::string& str);

inline bool should_log(Level logger_level, Level msg_level) noexcept {
    return msg_level >= logger_level;
}

} // namespace myspdlog