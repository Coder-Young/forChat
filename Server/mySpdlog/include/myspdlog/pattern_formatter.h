#pragma once

#include "formatter.h"
#include "level.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <ctime>

namespace myspdlog {

class pattern_formatter : public formatter {
public:
    explicit pattern_formatter(
        std::string pattern = "[%Y-%m-%d %H:%M:%S] [%l] %v"
    );
    
    ~pattern_formatter() override = default;
    
    void format(const details::log_msg& msg, fmt::memory_buffer& dest) override;
    std::unique_ptr<formatter> clone() const override;
    
    void set_pattern(std::string pattern);
    
public:
    class flag_formatter {
    public:
        virtual ~flag_formatter() = default;
        virtual void format(const details::log_msg& msg, 
                          const std::tm& tm_time, 
                          fmt::memory_buffer& dest) = 0;
        virtual std::unique_ptr<flag_formatter> clone() const = 0;
    };

private:    
    void compile_pattern();
    
    std::tm get_time(const details::log_msg& msg);
    
    std::string pattern_;                                      // pattern 字符串
    std::vector<std::unique_ptr<flag_formatter>> formatters_;  // flag_formatter 向量
    
    // 性能优化:时间缓存
    std::chrono::seconds last_log_secs_{0}; // 上次日志的秒数
    std::tm cached_tm_{};                   // 缓存的 tm 结构
};

} // namespace myspdlog