#pragma once

#include "../common.h"
#include <string>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

namespace myspdlog {
namespace details {

// 格式化时间为字符串
MYSPDLOG_API std::string format_time(
    const log_clock::time_point& tp, 
    const char* format = "%Y-%m-%d %H:%M:%S"
);

// 获取当前时间戳(毫秒)
MYSPDLOG_API int64_t get_timestamp_ms();

// 获取线程 ID
MYSPDLOG_API size_t get_thread_id();

// 字符串工具
MYSPDLOG_API std::string& ltrim(std::string& s);
MYSPDLOG_API std::string& rtrim(std::string& s);
MYSPDLOG_API std::string& trim(std::string& s);

} // namespace details
} // namespace myspdlog