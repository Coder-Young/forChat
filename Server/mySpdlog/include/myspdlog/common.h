#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <chrono>

namespace myspdlog {

constexpr const char* VERSION = "0.1.0";

#ifdef _WIN32
    #define MYSPDLOG_WINDOWS
#elif defined(__linux__)
    #define MYSPDLOG_LINUX
#elif defined(__APPLE__)
    #define MYSPDLOG_MACOS
#endif

// 导出符号定义(为未来动态库做准备)
#if defined(_WIN32) && defined(MYSPDLOG_SHARED)
    #ifdef MYSPDLOG_BUILD
        #define MYSPDLOG_API __declspec(dllexport)
    #else
        #define MYSPDLOG_API __declspec(dllimport)
    #endif
#else
    #define MYSPDLOG_API
#endif

using string_view_t = std::string;
using log_clock = std::chrono::system_clock;

} // namespace myspdlog