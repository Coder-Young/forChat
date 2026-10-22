#pragma once

#include "common.h"
#include "logger.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace myspdlog {

namespace details {
class thread_pool;
}

class MYSPDLOG_API registry {
public:
    registry(const registry&) = delete;
    registry& operator=(const registry&) = delete;
    
    static registry& instance();
    
    // Logger注册管理
    void register_logger(std::shared_ptr<logger> new_logger);
    std::shared_ptr<logger> get(const std::string& logger_name);
    void drop(const std::string& logger_name);
    void drop_all();
    
    // 默认Logger
    std::shared_ptr<logger> default_logger();
    void set_default_logger(std::shared_ptr<logger> new_default_logger);
    
    // 全局设置
    void set_level(Level log_level);
    void flush_all();

    // 线程池相关
    // 创建异步 logger 之前调用
    void init_thread_pool(size_t queue_size, size_t threads_n = 1);
    std::shared_ptr<details::thread_pool> thread_pool();
    void set_thread_pool(std::shared_ptr<details::thread_pool> tp);
    
private:
    registry();
    ~registry() = default;
    
    // 检查名称是否已存在
    void throw_if_exists_(const std::string& logger_name);

    void create_default_thread_pool_();
    
    std::mutex mutex_;  // loggers_&default_logger_
    std::unordered_map<std::string, std::shared_ptr<logger>> loggers_;  // Logger 映射表
    std::shared_ptr<logger> default_logger_; // 默认 logger

    std::shared_ptr<details::thread_pool> thread_pool_;
};

} // namespace myspdlog