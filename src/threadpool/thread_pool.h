#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <memory>
#include <type_traits>

namespace webserver {

/**
 * @brief 现代化的线程池实现
 * 
 * 特性：
 * - 支持任意可调用对象
 * - 返回std::future用于获取结果
 * - 异常安全
 * - 优雅关闭
 * - 线程安全的统计信息
 */
class ThreadPool {
public:
    /**
     * @brief 构造函数
     * @param numThreads 线程数量，默认为硬件并发数
     * @param maxQueueSize 最大队列长度，0表示无限制
     */
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency(),
                       size_t maxQueueSize = 0);
    
    /**
     * @brief 析构函数，等待所有任务完成并关闭线程池
     */
    ~ThreadPool() noexcept;
    
    // 禁用拷贝构造和拷贝赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    /**
     * @brief 提交任务到线程池
     * @tparam F 可调用对象类型
     * @tparam Args 参数类型
     * @param f 可调用对象
     * @param args 参数
     * @return std::future对象，用于获取结果
     * @throws std::runtime_error 线程池已关闭或队列已满时抛出异常
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>>;
    
    /**
     * @brief 提交任务到线程池（无返回值版本）
     * @tparam F 可调用对象类型
     * @param f 可调用对象
     * @throws std::runtime_error 线程池已关闭或队列已满时抛出异常
     */
    template<typename F>
    void submit(F&& f);
    
    /**
     * @brief 关闭线程池
     * @param waitForCompletion 是否等待所有任务完成
     */
    void shutdown(bool waitForCompletion = true) noexcept;
    
    /**
     * @brief 检查线程池是否正在运行
     * @return true表示正在运行
     */
    bool isRunning() const noexcept { return !shutdown_.load(); }
    
    /**
     * @brief 获取线程数量
     * @return 线程数量
     */
    size_t getThreadCount() const noexcept { return threads_.size(); }
    
    /**
     * @brief 获取队列中待处理任务数量
     * @return 任务数量
     */
    size_t getQueueSize() const noexcept;
    
    /**
     * @brief 获取活跃线程数量
     * @return 活跃线程数量
     */
    size_t getActiveThreadCount() const noexcept { return activeThreads_.load(); }
    
    /**
     * @brief 获取已完成任务总数
     * @return 任务总数
     */
    uint64_t getCompletedTaskCount() const noexcept { return completedTasks_.load(); }

private:
    /**
     * @brief 工作线程函数
     */
    void workerThread();
    
private:
    std::vector<std::thread> threads_;                    ///< 工作线程
    std::queue<std::function<void()>> tasks_;            ///< 任务队列
    
    mutable std::mutex queueMutex_;                      ///< 队列互斥锁
    std::condition_variable condition_;                   ///< 条件变量
    
    std::atomic<bool> shutdown_{false};                  ///< 关闭标志
    size_t maxQueueSize_;                                ///< 最大队列长度
    
    // 统计信息
    std::atomic<size_t> activeThreads_{0};              ///< 活跃线程数
    std::atomic<uint64_t> completedTasks_{0};           ///< 已完成任务数
};

// 模板方法实现
template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result_t<F, Args...>> {
    
    using ReturnType = typename std::invoke_result_t<F, Args...>;
    
    if (shutdown_.load()) {
        throw std::runtime_error("ThreadPool is shutdown");
    }
    
    // 创建packaged_task
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    auto future = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        if (shutdown_.load()) {
            throw std::runtime_error("ThreadPool is shutdown");
        }
        
        if (maxQueueSize_ > 0 && tasks_.size() >= maxQueueSize_) {
            throw std::runtime_error("ThreadPool queue is full");
        }
        
        tasks_.emplace([task]() { (*task)(); });
    }
    
    condition_.notify_one();
    return future;
}

template<typename F>
void ThreadPool::submit(F&& f) {
    if (shutdown_.load()) {
        throw std::runtime_error("ThreadPool is shutdown");
    }
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        if (shutdown_.load()) {
            throw std::runtime_error("ThreadPool is shutdown");
        }
        
        if (maxQueueSize_ > 0 && tasks_.size() >= maxQueueSize_) {
            throw std::runtime_error("ThreadPool queue is full");
        }
        
        tasks_.emplace(std::forward<F>(f));
    }
    
    condition_.notify_one();
}

} // namespace webserver