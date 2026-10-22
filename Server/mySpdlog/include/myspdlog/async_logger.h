#pragma once

#include "logger.h"
#include "details/thread_pool.h"
#include <memory>

namespace myspdlog {

// 前向声明
namespace details {
class thread_pool;
}

// 溢出策略枚举
// 参考 spdlog 设计:队列满时的两种处理方式
enum class async_overflow_policy {
    block,          // 阻塞调用者，等待队列有空闲槽位
    overrun_oldest  // 立即覆盖最旧消息
};

// async_logger异步日志记录器
class MYSPDLOG_API async_logger final 
    : public logger
    , public std::enable_shared_from_this<async_logger> 
{
    // thread_pool需要访问backend_sink_it_()
    friend class details::thread_pool;

public:
    template<typename It>
    async_logger(
        std::string name,
        It begin,
        It end,
        std::weak_ptr<details::thread_pool> tp,
        async_overflow_policy policy = async_overflow_policy::block
    )
        : logger(std::move(name), begin, end)
        , thread_pool_(std::move(tp))
        , overflow_policy_(policy)
    {}

    async_logger(
        std::string name,
        sinks::sink_ptr single_sink,
        std::weak_ptr<details::thread_pool> tp,
        async_overflow_policy policy = async_overflow_policy::block
    );

    async_logger(
        std::string name,
        std::vector<sinks::sink_ptr> sinks,
        std::weak_ptr<details::thread_pool> tp,
        async_overflow_policy policy = async_overflow_policy::block
    );

    ~async_logger() override = default;

    async_logger(const async_logger&) = delete;
    async_logger& operator=(const async_logger&) = delete;

protected:
    void sink_it_(const details::log_msg& msg) override;

    void flush_() override;

    void backend_sink_it_(const details::log_msg& msg);

    void backend_flush_();

private:
    std::weak_ptr<details::thread_pool> thread_pool_;  // 线程池(弱引用)
    async_overflow_policy overflow_policy_;            // 溢出策略
};

} // namespace myspdlog