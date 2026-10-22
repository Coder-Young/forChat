#pragma once

#include "common.h"
#include "level.h"
#include "logger.h"
#include "registry.h"
#include "sinks/console_sink.h"
#include "sinks/color_console_sink.h"
#include "sinks/file_sink.h"
#include "sinks/rotating_file_sink.h"
#include <fmt/format.h>
#include <memory>
#include <string>

namespace myspdlog {

// 获取 logger
inline std::shared_ptr<logger> get(const std::string& name) {
    return registry::instance().get(name);
}

// 注册logger
inline void register_logger(std::shared_ptr<logger> logger) {
    registry::instance().register_logger(std::move(logger));
}

// 移除logger
inline void drop(const std::string& name) {
    registry::instance().drop(name);
}

// 移除所有logger
inline void drop_all() {
    registry::instance().drop_all();
}

// 获取默认logger
inline std::shared_ptr<logger> default_logger() {
    return registry::instance().default_logger();
}

// 设置默认logger
inline void set_default_logger(std::shared_ptr<logger> new_default_logger) {
    registry::instance().set_default_logger(std::move(new_default_logger));
}

// 设置所有logger级别
inline void set_level(Level log_level) {
    registry::instance().set_level(log_level);
}

// 刷新所有logger
inline void flush_all() {
    registry::instance().flush_all();
}


inline std::shared_ptr<logger> stdout_color_mt(const std::string& logger_name) {
    auto sink = std::make_shared<sinks::color_console_sink_mt>();
    auto new_logger = std::make_shared<logger>(logger_name, sink);
    register_logger(new_logger);
    return new_logger;
}

inline std::shared_ptr<logger> stderr_color_mt(const std::string& logger_name) {
    auto sink = std::make_shared<sinks::color_stderr_sink_mt>();
    auto new_logger = std::make_shared<logger>(logger_name, sink);
    register_logger(new_logger);
    return new_logger;
}

inline std::shared_ptr<logger> stdout_mt(const std::string& logger_name) {
    auto sink = std::make_shared<sinks::console_sink_mt>();
    auto new_logger = std::make_shared<logger>(logger_name, sink);
    register_logger(new_logger);
    return new_logger;
}

inline std::shared_ptr<logger> basic_logger_mt(
    const std::string& logger_name,
    const std::string& filename,
    bool truncate = false
) {
    auto sink = std::make_shared<sinks::file_sink_mt>(filename, truncate);
    auto new_logger = std::make_shared<logger>(logger_name, sink);
    register_logger(new_logger);
    return new_logger;
}

inline std::shared_ptr<logger> rotating_logger_mt(
    const std::string& logger_name,
    const std::string& filename,
    size_t max_size,
    size_t max_files
) {
    auto sink = std::make_shared<sinks::rotating_file_sink_mt>(filename, max_size, max_files);
    auto new_logger = std::make_shared<logger>(logger_name, sink);
    register_logger(new_logger);
    return new_logger;
}


template<typename... Args>
inline void trace(fmt::format_string<Args...> fmt, Args&&... args) {
    default_logger()->trace(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args&&... args) {
    default_logger()->debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args&&... args) {
    default_logger()->info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void warn(fmt::format_string<Args...> fmt, Args&&... args) {
    default_logger()->warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args&&... args) {
    default_logger()->error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void critical(fmt::format_string<Args...> fmt, Args&&... args) {
    default_logger()->critical(fmt, std::forward<Args>(args)...);
}

} // namespace myspdlog