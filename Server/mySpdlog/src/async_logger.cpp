#include "../include/myspdlog/async_logger.h"
#include <stdexcept>

namespace myspdlog {

async_logger::async_logger(
    std::string name,
    sinks::sink_ptr single_sink,
    std::weak_ptr<details::thread_pool> tp,
    async_overflow_policy policy
)
    : logger(std::move(name), std::move(single_sink))
    , thread_pool_(std::move(tp))
    , overflow_policy_(policy)
{}

async_logger::async_logger(
    std::string name,
    std::vector<sinks::sink_ptr> sinks,
    std::weak_ptr<details::thread_pool> tp,
    async_overflow_policy policy
)
    : logger(std::move(name), std::move(sinks))
    , thread_pool_(std::move(tp))
    , overflow_policy_(policy)
{}

// 用户线程调用
void async_logger::sink_it_(const details::log_msg& msg) {
    if (auto pool_ptr = thread_pool_.lock()) {
        if (overflow_policy_ == async_overflow_policy::block) {
            pool_ptr->post_log(shared_from_this(), msg);
        } else {
            pool_ptr->post_log_nowait(shared_from_this(), msg);
        }
    } else {
        throw std::runtime_error(
            "async_logger::sink_it_: thread pool doesn't exist anymore"
        );
    }
}

void async_logger::flush_() {
    if (auto pool_ptr = thread_pool_.lock()) {
        pool_ptr->post_flush(shared_from_this());
    } else {
        throw std::runtime_error(
            "async_logger::flush: thread pool doesn't exist anymore"
        );
    }
}

// 后台线程调用
void async_logger::backend_sink_it_(const details::log_msg& msg) {
    for (auto& sink : sinks_) {
        if (sink->should_log(msg.lvl)) {
            sink->log(msg);
        }
    }

    if (msg.lvl >= flush_level_) {
        backend_flush_();
    }
}

// 后台线程调用
void async_logger::backend_flush_() {
    for (auto& sink : sinks_) {
        sink->flush();
    }
}

} // namespace myspdlog