#pragma once

#include "common.h"
#include "details/log_msg.h"
#include <fmt/format.h>
#include <memory>

namespace myspdlog {

// formatter 抽象接口
class formatter {
public:
    virtual ~formatter() = default;
    
    // 格式化日志消息到缓冲区
    virtual void format(const details::log_msg& msg, fmt::memory_buffer& dest) = 0;
    
    // 创建 formatter 的副本
    virtual std::unique_ptr<formatter> clone() const = 0;
};

} // namespace myspdlog