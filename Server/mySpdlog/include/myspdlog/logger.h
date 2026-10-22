#pragma once

#include "common.h"
#include "level.h"
#include "sinks/base_sink.h"
#include "details/log_msg.h"
#include <fmt/format.h>
#include <vector>
#include <memory>
#include <string>

namespace myspdlog {
// 参考 spdlog 设计:
// 1. 持有多个 sink,每次日志调用会遍历所有sink
// 2. 支持日志级别过滤
// 3. 使用fmt库和变参模板提供灵活的日志接口
class MYSPDLOG_API logger {
public:
    // 构造函数
    explicit logger(std::string name);
    logger(std::string name, sinks::sink_ptr single_sink);
    logger(std::string name, std::vector<sinks::sink_ptr> sinks);

    virtual ~logger() = default;

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;

    // ========== 日志接口 ==========
    template<typename... Args>
    void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        log(Level::LOG_TRACE, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        log(Level::LOG_DEBUG, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) {
        log(Level::LOG_INFO, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        log(Level::LOG_WARN, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) {
        log(Level::LOG_ERROR, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        log(Level::LOG_CRITICAL, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(Level lvl, fmt::format_string<Args...> fmt, Args&&... args) {
        if (!should_log(lvl)) {
            return;
        }

        // 格式化消息
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);

        // 创建 log_msg
        details::log_msg msg(
        name_,
        lvl,
        string_view_t(buf.data(), buf.size())
        );

        // 输出到所有 sink
        sink_it_(msg);
    }

    void add_sink(sinks::sink_ptr sink);
    void remove_sink(sinks::sink_ptr sink);
    std::vector<sinks::sink_ptr>& sinks();
    const std::vector<sinks::sink_ptr>& sinks() const;

    void set_level(Level log_level);
    Level get_level() const;
    bool should_log(Level msg_level) const;

    void flush();
    void flush_on(Level log_level);

    const std::string& name() const;

public:
        // 将消息输出到所有sink
    virtual void sink_it_(const details::log_msg& msg);
    virtual void flush_();

protected:

    
    std::string name_;                     // Logger 名称
    std::vector<sinks::sink_ptr> sinks_;   // Sink 列表
    Level level_{Level::LOG_TRACE};        // 日志级别
    Level flush_level_{Level::LOG_OFF};    // 自动刷新级别
};

} // namespace myspdlog