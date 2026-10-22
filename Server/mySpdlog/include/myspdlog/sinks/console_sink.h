#pragma once

#include "base_sink.h"
#include <iostream>
#include <mutex>

namespace myspdlog {
namespace sinks {

// 控制台Sink
template<typename ConsoleMutex>
class Console_sink : public Base_sink<ConsoleMutex> {
public:
    Console_sink() = default;
    ~Console_sink() override = default;
    
protected:
    void sink_it_(const details::log_msg& msg) override {
        fmt::memory_buffer formatted;
        this->format_message(msg, formatted);
        
        // 输出到 stdout
        std::cout.write(formatted.data(), formatted.size());
    }
    
    void flush_() override {
        std::cout << std::flush;
    }
};

typedef Console_sink<std::mutex> console_sink_mt;
// 单线程版本
typedef Console_sink<null_mutex> console_sink_st;

template<typename ConsoleMutex>
class Stderr_sink : public Base_sink<ConsoleMutex> {
public:
    Stderr_sink() = default;
    ~Stderr_sink() override = default;
    
protected:
    void sink_it_(const details::log_msg& msg) override {
        fmt::memory_buffer formatted;
        this->format_message(msg, formatted);
        
        std::cerr.write(formatted.data(), formatted.size());
    }
    
    void flush_() override {
        std::cerr << std::flush;
    }
};

typedef Stderr_sink<std::mutex> stderr_sink_mt;
typedef Stderr_sink<null_mutex> stderr_sink_st;

} // namespace sinks
} // namespace myspdlog