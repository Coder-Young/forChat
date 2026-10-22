#include "../../include/myspdlog/details/thread_pool.h"
#include "../../include/myspdlog/logger.h"

namespace myspdlog {
namespace details {

thread_pool::thread_pool(size_t queue_size, size_t threads_n)
    : q_(queue_size)
{
    if (threads_n == 0 || threads_n > 1000) {
        throw std::invalid_argument("thread_pool: threads_n must be 1-1000");
    }
    
    for (size_t i = 0; i < threads_n; ++i) {
        threads_.emplace_back([this] { this->worker_loop_(); });
    }
}

thread_pool::~thread_pool() {
    try {
        for (size_t i = 0; i < threads_.size(); ++i) {
            async_msg terminate_msg;
            terminate_msg.msg_type = async_msg_type::terminate;
            q_.enqueue(std::move(terminate_msg));
        }
        
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    } catch (...) {
    }
}

void thread_pool::post_log(std::shared_ptr<logger> &&logger_ptr, const log_msg& msg) {
    async_msg async_m(async_msg_type::log, std::move(logger_ptr), msg);
    q_.enqueue(std::move(async_m));
}

void thread_pool::post_log_nowait(std::shared_ptr<logger> &&logger_ptr, const log_msg& msg) {
    async_msg async_m(async_msg_type::log, std::move(logger_ptr), msg);
    q_.enqueue_nowait(std::move(async_m));
}

void thread_pool::post_flush(std::shared_ptr<logger> &&logger_ptr) {
    async_msg flush_msg(async_msg_type::flush, std::move(logger_ptr));
    q_.enqueue(std::move(flush_msg));
}

#if 0
void thread_pool::post_flush(std::shared_ptr<logger> &&logger_ptr) {
    async_msg flush_msg;
    flush_msg.msg_type = async_msg_type::flush;
    flush_msg.logger_ptr = logger_ptr;
    q_.enqueue(std::move(flush_msg));
}
#endif

size_t thread_pool::overrun_counter() {
    return q_.overrun_counter();
}

void thread_pool::worker_loop_() {
    while (process_next_msg_()) {
    }
}

bool thread_pool::process_next_msg_() {
    async_msg incoming_async_msg;
    
    if (!q_.dequeue_for(incoming_async_msg, std::chrono::seconds(10))) {
        return true;
    }
    
    switch (incoming_async_msg.msg_type) {
        case async_msg_type::log: {
            if (incoming_async_msg.worker_ptr) {
                incoming_async_msg.worker_ptr->sink_it_(incoming_async_msg);
            }
            return true;
        }
        
        case async_msg_type::flush: {
            if (incoming_async_msg.worker_ptr) {
                incoming_async_msg.worker_ptr->flush();
            }
            return true;
        }
        
        case async_msg_type::terminate: {
            return false;
        }
    }
    
    return true;
}

} // namespace details
} // namespace myspdlog