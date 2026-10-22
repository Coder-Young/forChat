#pragma once

#include "circular_q.h"
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace myspdlog {
    namespace details {

    template<typename T>
    class mpmc_blocking_queue {
        public:
        using item_type = T;

        explicit mpmc_blocking_queue(size_t max_items)
        : q_(max_items)
        {}

        mpmc_blocking_queue(const mpmc_blocking_queue&) = delete;
        mpmc_blocking_queue& operator=(const mpmc_blocking_queue&) = delete;

        void enqueue(T&& item) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                pop_cv_.wait(lock, [this] { return !this->q_.full(); });
                q_.push_back(std::move(item));
            }
            push_cv_.notify_one();
        }

        void enqueue_nowait(T&& item) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                q_.push_back(std::move(item));
            }
            push_cv_.notify_one();
        }

        bool dequeue_for(T& popped_item, std::chrono::milliseconds wait_duration) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!push_cv_.wait_for(lock, wait_duration, [this] { 
                return !this->q_.empty(); 
            }))
            {
                return false;
            }

            popped_item = std::move(q_.front());
            q_.pop_front();

            pop_cv_.notify_one();
            return true;
        }

        size_t overrun_counter() {
            std::unique_lock<std::mutex> lock(mutex_);
            return q_.overrun_counter();
        }

        size_t size() {
            std::unique_lock<std::mutex> lock(mutex_);
            return q_.size();
        }

        private:
        std::mutex mutex_;                      // 保护队列
        std::condition_variable push_cv_;       // 通知消费者:有新元素
        std::condition_variable pop_cv_;        // 通知生产者:有空闲槽位
        circular_q<T> q_;                       // 循环队列
    };

    } // namespace details
} // namespace myspdlog