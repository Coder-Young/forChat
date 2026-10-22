#pragma once

#include "../common.h"
#include "../details/log_msg.h"
#include "../formatter.h"
#include "../pattern_formatter.h"
#include <mutex>
#include <memory>

namespace myspdlog {
namespace sinks {

// Sink接口类
class Sink {
public:
    virtual ~Sink() = default;
    
    // 输出日志
    virtual void log(const details::log_msg& msg) = 0;
    
    // 刷新缓冲区
    virtual void flush() = 0;
    
    // 日志级别相关
    virtual void set_level(Level log_level) = 0;
    virtual Level get_level() const = 0;
    
    // 是否应该输出
    virtual bool should_log(Level msg_level) const = 0;

    // Formatter 相关接口
    virtual void set_formatter(std::unique_ptr<formatter> sink_formatter) = 0;
};

// Mutex可以是null_mutex用于实现单线程
template<typename Mutex>
class Base_sink : public Sink {
public:
    Base_sink() : level_(Level::LOG_TRACE)
                , formatter_(std::make_unique<pattern_formatter>())
    {}
    
    Base_sink(const Base_sink&) = delete;
    Base_sink& operator=(const Base_sink&) = delete;
    
    void log(const details::log_msg& msg) override {
        std::lock_guard<Mutex> lock(mutex_);
        sink_it_(msg);
    }
    
    void flush() override {
        std::lock_guard<Mutex> lock(mutex_);
        flush_();
    }
    
    void set_level(Level log_level) override {
        std::lock_guard<Mutex> lock(mutex_);
        level_ = log_level;
    }
    
    Level get_level() const override {
        std::lock_guard<Mutex> lock(mutex_);
        return level_;
    }
    
    bool should_log(Level msg_level) const override {
        return msg_level >= level_;
    }

    void set_formatter(std::unique_ptr<formatter> sink_formatter) override {
        std::lock_guard<Mutex> lock(mutex_);
        formatter_ = std::move(sink_formatter);
    }
    
protected:
    // 子类需要实现的核心方法(加锁)
    virtual void sink_it_(const details::log_msg& msg) = 0;
    virtual void flush_() = 0;

    // 格式化日志消息
    void format_message(const details::log_msg& msg, fmt::memory_buffer& dest) {
        formatter_->format(msg, dest);
    }
    
    mutable Mutex mutex_;
    Level level_;
    std::unique_ptr<formatter> formatter_;
};

struct null_mutex {
    void lock() {}
    void unlock() {}
};

typedef std::shared_ptr<Sink> sink_ptr;

} // namespace sinks
} // namespace myspdlog