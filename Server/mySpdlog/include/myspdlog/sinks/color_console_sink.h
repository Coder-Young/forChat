#pragma once

#include "base_sink.h"
#include <iostream>
#include <mutex>
#include <array>

namespace myspdlog {
namespace sinks {

// ANSI 颜色码
// 参考: https://en.wikipedia.org/wiki/ANSI_escape_code
namespace color {
    constexpr const char* reset   = "\033[0m";
    constexpr const char* bold    = "\033[1m";
    constexpr const char* white   = "\033[37m";
    constexpr const char* green   = "\033[32m";
    constexpr const char* yellow  = "\033[33m";
    constexpr const char* red     = "\033[31m";
    constexpr const char* magenta = "\033[35m";
    constexpr const char* cyan    = "\033[36m";
}

// color_console_sink:带颜色支持的控制台 Sink
// 使用 ANSI 转义码为不同级别的日志添加颜色
template<typename ConsoleMutex>
class color_console_sink : public Base_sink<ConsoleMutex> {
public:
    color_console_sink() {
        colors_[static_cast<int>(Level::LOG_TRACE)] = color::white;
        colors_[static_cast<int>(Level::LOG_DEBUG)] = color::cyan;
        colors_[static_cast<int>(Level::LOG_INFO)] = color::green;
        colors_[static_cast<int>(Level::LOG_WARN)] = color::yellow;
        colors_[static_cast<int>(Level::LOG_ERROR)] = color::red;
        colors_[static_cast<int>(Level::LOG_CRITICAL)] = std::string(color::bold) + color::red;
    }
    
    ~color_console_sink() override = default;
    
protected:
    void sink_it_(const details::log_msg& msg) override {
        fmt::memory_buffer formatted;
        this->format_message(msg, formatted);
        
        // 添加颜色前缀
        const std::string& prefix = colors_[static_cast<int>(msg.lvl)];
        
        // 输出: 颜色前缀 + 消息 + 颜色重置
        std::cout << prefix;
        std::cout.write(formatted.data(), formatted.size());
        std::cout << color::reset;
    }
    
    void flush_() override {
        std::cout << std::flush;
    }
    
private:
    std::array<std::string, 7> colors_;  // 7 个级别的颜色
};

typedef color_console_sink<std::mutex> color_console_sink_mt;
typedef color_console_sink<null_mutex> color_console_sink_st;

// stderr 彩色版本
template<typename ConsoleMutex>
class color_stderr_sink : public Base_sink<ConsoleMutex> {
public:
    color_stderr_sink() {
        colors_[static_cast<int>(Level::LOG_TRACE)] = color::white;
        colors_[static_cast<int>(Level::LOG_DEBUG)] = color::cyan;
        colors_[static_cast<int>(Level::LOG_INFO)] = color::green;
        colors_[static_cast<int>(Level::LOG_WARN)] = color::yellow;
        colors_[static_cast<int>(Level::LOG_ERROR)] = color::red;
        colors_[static_cast<int>(Level::LOG_CRITICAL)] = std::string(color::bold) + color::red;
    }
    
    ~color_stderr_sink() override = default;
    
protected:
    void sink_it_(const details::log_msg& msg) override {
        fmt::memory_buffer formatted;
        this->format_message(msg, formatted);
        
        const std::string& prefix = colors_[static_cast<int>(msg.lvl)];
        
        std::cerr << prefix;
        std::cerr.write(formatted.data(), formatted.size());
        std::cerr << color::reset;
    }
    
    void flush_() override {
        std::cerr << std::flush;
    }
    
private:
    std::array<std::string, 7> colors_;
};

typedef color_stderr_sink<std::mutex> color_stderr_sink_mt;
typedef color_stderr_sink<null_mutex> color_stderr_sink_st;

} // namespace sinks
} // namespace myspdlog