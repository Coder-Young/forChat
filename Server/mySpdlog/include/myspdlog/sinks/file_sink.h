#pragma once

#include "base_sink.h"
#include <fstream>
#include <string>
#include <mutex>

namespace myspdlog {
namespace sinks {

// file_sink:基础文件输出 Sink
template<typename Mutex>
class file_sink : public Base_sink<Mutex> {
public:
    // truncate: true=覆盖文件, false=追加到文件末尾
    explicit file_sink(const std::string& filename, bool truncate = false) {
        auto mode = truncate ? std::ios::trunc : std::ios::app;
        file_.open(filename, std::ios::out | mode);
        
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }
    
    ~file_sink() override {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
protected:
    void sink_it_(const details::log_msg& msg) override {
        fmt::memory_buffer formatted;
        this->format_message(msg, formatted);
        
        file_.write(formatted.data(), formatted.size());
    }
    
    void flush_() override {
        file_.flush();
    }
    
private:
    std::ofstream file_;
};

typedef file_sink<std::mutex> file_sink_mt;
typedef file_sink<null_mutex> file_sink_st;

} // namespace sinks
} // namespace myspdlog