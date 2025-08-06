#include "thread_pool.h"
#include <iostream>

namespace webserver {

ThreadPool::ThreadPool(size_t numThreads, size_t maxQueueSize)
    : maxQueueSize_(maxQueueSize) {
    
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4; // 默认值
        }
    }
    
    threads_.reserve(numThreads);
    
    try {
        for (size_t i = 0; i < numThreads; ++i) {
            threads_.emplace_back(&ThreadPool::workerThread, this);
        }
    } catch (...) {
        shutdown(false);
        throw;
    }
}

ThreadPool::~ThreadPool() noexcept {
    shutdown(true);
}

void ThreadPool::shutdown(bool waitForCompletion) noexcept {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (shutdown_.load()) {
            return; // 已经关闭
        }
        shutdown_.store(true);
    }
    
    condition_.notify_all();
    
    if (waitForCompletion) {
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                try {
                    thread.join();
                } catch (...) {
                    // 忽略join异常
                }
            }
        }
    } else {
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.detach();
            }
        }
    }
    
    threads_.clear();
}

size_t ThreadPool::getQueueSize() const noexcept {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            condition_.wait(lock, [this] {
                return shutdown_.load() || !tasks_.empty();
            });
            
            if (shutdown_.load() && tasks_.empty()) {
                break;
            }
            
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        if (task) {
            activeThreads_.fetch_add(1);
            
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in thread pool task" << std::endl;
            }
            
            activeThreads_.fetch_sub(1);
            completedTasks_.fetch_add(1);
        }
    }
}

} // namespace webserver