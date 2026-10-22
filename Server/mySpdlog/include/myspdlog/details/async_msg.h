#pragma once

#include "log_msg.h"
#include <memory>
#include <string>

namespace myspdlog {
class logger;
namespace details {

// 异步消息类型
enum class async_msg_type {
    log,        // 普通日志消息
    flush,      // 刷新请求
    terminate   // 终止线程池
};

struct log_msg_buffer : log_msg {
    std::string buffer;
    
    log_msg_buffer() = default;
    
    explicit log_msg_buffer(const log_msg& msg)
        : log_msg(msg)
        , buffer(msg.payload.data(), msg.payload.size())
    {
        payload = string_view_t(buffer);
    }
};

// async_msg: 异步日志消息
// 参考 spdlog 设计:继承 log_msg_buffer + logger 的 shared_ptr
struct async_msg : log_msg_buffer {
    async_msg_type msg_type{async_msg_type::log};
    
    std::shared_ptr<logger> worker_ptr;
    
    async_msg() = default;
    ~async_msg() = default;

    async_msg(const async_msg &) = delete;
    
    async_msg(async_msg_type type, std::shared_ptr<logger> &&worker, const log_msg& msg)
        : log_msg_buffer(msg)
        , msg_type(type)
        , worker_ptr(std::move(worker))
    {}

    async_msg(async_msg_type the_type,std::shared_ptr<logger> &&worker)
        : log_msg_buffer{},
          msg_type{the_type},
          worker_ptr{std::move(worker)} {}

    explicit async_msg(async_msg_type the_type)
        : async_msg{the_type,nullptr} {}
};

} // namespace details
} // namespace myspdlog