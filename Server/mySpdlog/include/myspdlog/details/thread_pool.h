#pragma once

#include "../common.h"
#include "mpmc_blocking_q.h"
#include "async_msg.h"
#include <thread>
#include <vector>
#include <functional>

namespace myspdlog {
namespace details {

// 异步日志的线程池
// 参考spdlog设计:管理工作线程 + MPMC 队列
class MYSPDLOG_API thread_pool {
public:
    using item_type = async_msg;
    using q_type = mpmc_blocking_queue<item_type>;
    
    thread_pool(size_t queue_size, size_t threads_n);
    
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    
    ~thread_pool();
    
    // 投递日志消息(阻塞模式)
    void post_log(std::shared_ptr<logger> &&logger_ptr, const log_msg& msg);
    
    // 投递日志消息(非阻塞模式,队列满时覆盖)
    void post_log_nowait(std::shared_ptr<logger> &&logger_ptr, const log_msg& msg);
    
    // 投递刷新请求
    void post_flush(std::shared_ptr<logger> &&logger_ptr);
    
    // 获取溢出计数
    size_t overrun_counter();
    
private:
    // 工作线程主循环
    void worker_loop_();
    
    // 处理下一条消息(返回 false 表示应该退出)
    bool process_next_msg_();
    
    q_type q_;                          // MPMC 队列
    std::vector<std::thread> threads_;  // 工作线程
};

} // namespace details
} // namespace myspdlog