#pragma once

#include <vector>
#include <cstddef>
#include <cassert>

namespace myspdlog {
    namespace details {

    // 参考 spdlog 设计:预分配所有槽位,使用 head/tail 指针管理
    template<typename T>
    class circular_q {
        public:
        using value_type = T;

        explicit circular_q(size_t max_items)
        : max_items_(max_items + 1)
        , v_(max_items_)
        , head_(0)
        , tail_(0)
        , overrun_counter_(0)
        {}

        circular_q(const circular_q&) = delete;
        circular_q& operator=(const circular_q&) = delete;

        void push_back(T&& item) {
            v_[tail_] = std::move(item);
            tail_ = (tail_ + 1) % max_items_;

            if (tail_ == head_) {
                head_ = (head_ + 1) % max_items_;
                ++overrun_counter_;
            }
        }

        const T& front() const {
            return v_[head_];
        }

        T& front() {
            return v_[head_];
        }

        void pop_front() {
            head_ = (head_ + 1) % max_items_;
        }

        bool empty() const {
            return head_ == tail_;
        }

        bool full() const {
            return ((tail_ + 1) % max_items_) == head_;
        }

        size_t size() const {
            if (tail_ >= head_) {
                return tail_ - head_;
            } else {
                return max_items_ - (head_ - tail_);
            }
        }

        size_t capacity() const {
            return max_items_ - 1;
        }

        // 溢出次数
        size_t overrun_counter() const {
            return overrun_counter_;
        }

        private:
        size_t max_items_;          // 数组大小(capacity + 1)
        std::vector<T> v_;          // 存储数据
        size_t head_;               // 队首索引
        size_t tail_;               // 队尾索引
        size_t overrun_counter_;    // 溢出计数
    };

    } // namespace details
} // namespace myspdlog