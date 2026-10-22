#include "../include/myspdlog/registry.h"
#include "../include/myspdlog/sinks/color_console_sink.h"
#include "../include/myspdlog/details/thread_pool.h"
#include <stdexcept>

namespace myspdlog {

registry::registry() {
    // 默认logger 彩色控制台输出
    auto console_sink = std::make_shared<sinks::color_console_sink_mt>();
    default_logger_ = std::make_shared<logger>("", console_sink);
    default_logger_->set_level(Level::LOG_INFO);
}

registry& registry::instance() {
    static registry s_instance;
    return s_instance;
}

void registry::register_logger(std::shared_ptr<logger> new_logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    throw_if_exists_(new_logger->name());
    loggers_[new_logger->name()] = std::move(new_logger);
}

std::shared_ptr<logger> registry::get(const std::string& logger_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loggers_.find(logger_name);
    if (it != loggers_.end()) {
        return it->second;
    }
    return nullptr;
}

void registry::drop(const std::string& logger_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto is_default_logger = default_logger_ && default_logger_->name() == logger_name;
    loggers_.erase(logger_name);
    if (is_default_logger) {
        default_logger_.reset();
    }
}

void registry::drop_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    loggers_.clear();
    default_logger_.reset();
}

std::shared_ptr<logger> registry::default_logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    return default_logger_;
}

void registry::set_default_logger(std::shared_ptr<logger> new_default_logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    if ( new_default_logger != nullptr ){
        loggers_[new_default_logger->name()] = new_default_logger; 
    }
    default_logger_ = std::move(new_default_logger);
}

void registry::set_level(Level log_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    default_logger_->set_level(log_level);
    
    for (auto& pair : loggers_) {
        pair.second->set_level(log_level);
    }
}

void registry::flush_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    default_logger_->flush();
    
    for (auto& pair : loggers_) {
        pair.second->flush();
    }
}

void registry::throw_if_exists_(const std::string& logger_name) {
    if (loggers_.find(logger_name) != loggers_.end()) {
        throw std::runtime_error("Logger with name '" + logger_name + "' already exists");
    }
}

// ========== 线程池相关 ==========
void registry::init_thread_pool(size_t queue_size, size_t threads_n) {
    std::lock_guard<std::mutex> lock(mutex_);
    thread_pool_ = std::make_shared<details::thread_pool>(queue_size, threads_n);
}

std::shared_ptr<details::thread_pool>
registry::thread_pool() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!thread_pool_) {
        create_default_thread_pool_();
    }
    
    return thread_pool_;
}

void registry::set_thread_pool(std::shared_ptr<details::thread_pool> tp) {
    std::lock_guard<std::mutex> lock(mutex_);
    thread_pool_ = std::move(tp);
}

void registry::create_default_thread_pool_() {
    constexpr size_t default_queue_size = 8192;
    constexpr size_t default_threads = 1;
    
    thread_pool_ = std::make_shared<details::thread_pool>(
        default_queue_size, 
        default_threads
    );
}

} // namespace myspdlog