#pragma once
/**
注意,SafeQueue一定需要线程安全,因为每个队列都会被两个线程所使用
一个是解封装器线程,负责push pkt
另一个是解码线程,负责 pop pkt
当解码线程需要pop的时候会wait,
只有当push一个pkt的时候,才会唤醒他
我们当然可以不适用wait,而是return false的时候 continue
但是这样会陷入循环中,知道有数据,但是这样会浪费cpu资源
*/
#include <condition_variable>
#include <chrono>
#include <deque>
#include <mutex>
#include <utility>

template <typename T>
class SafeQueue {
public:
    SafeQueue() = default;
    SafeQueue(const SafeQueue&) = delete;
    SafeQueue& operator=(const SafeQueue&) = delete;

    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (aborted_) {
                return;
            }
            queue_.push_back(std::move(value));
        }
        cv_.notify_one();
    }

    //只有当解封装器push一个pkt指针,我们才唤醒
    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return aborted_ || !queue_.empty(); });
        if (aborted_ && queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    //限定超时的pop,如果超过某段时间,返回false
    bool popFor(T& out, uint32_t timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        //被唤醒,如果不是超时,返回true,如果超时,返回false
        const bool ready = cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return aborted_ || !queue_.empty(); });
        if (!ready) {
            return false;
        }
        if (aborted_ && queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            aborted_ = true;
        }
        //唤醒所有wait的线程,也就是解码器线程
        cv_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        aborted_ = false;
        queue_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<T> queue_;
    bool aborted_ = false;
};
