#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "../utils/logger.h"
#include "../network/connection_manager.h"
#include "../threadpool/thread_pool.h"
#include "../config/server_config.h"

namespace webserver {

/**
 * @brief 现代化的Web服务器核心类
 * 
 * 采用RAII设计原则，使用智能指针管理资源
 * 支持优雅关闭和异常安全的操作
 */
class WebServer {
public:
    /**
     * @brief 构造函数
     * @param config 服务器配置
     */
    explicit WebServer(const ServerConfig& config);
    
    /**
     * @brief 析构函数，确保资源正确释放
     */
    ~WebServer() noexcept;
    
    // 禁用拷贝构造和拷贝赋值
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    
    // 启用移动构造和移动赋值
    WebServer(WebServer&&) noexcept = default;
    WebServer& operator=(WebServer&&) noexcept = default;
    
    /**
     * @brief 启动服务器
     * @throws std::runtime_error 启动失败时抛出异常
     */
    void start();
    
    /**
     * @brief 停止服务器
     * @param graceful 是否优雅关闭
     */
    void stop(bool graceful = true) noexcept;
    
    /**
     * @brief 检查服务器是否正在运行
     * @return true表示正在运行
     */
    bool isRunning() const noexcept { return running_.load(); }
    
    /**
     * @brief 获取服务器统计信息
     * @return 统计信息字符串
     */
    std::string getStats() const;

private:
    /**
     * @brief 初始化监听套接字
     */
    void initializeListenSocket();
    
    /**
     * @brief 初始化epoll实例
     */
    void initializeEpoll();
    
    /**
     * @brief 设置信号处理
     */
    void setupSignalHandling();
    
    /**
     * @brief 主事件循环
     */
    void eventLoop();
    
    /**
     * @brief 处理新连接
     */
    void handleNewConnection();
    
    /**
     * @brief 处理客户端事件
     * @param fd 文件描述符
     * @param events 事件类型
     */
    void handleClientEvent(int fd, uint32_t events);
    
    /**
     * @brief 信号处理函数
     * @param signum 信号编号
     */
    static void signalHandler(int signum);
    
private:
    ServerConfig config_;                           ///< 服务器配置
    std::unique_ptr<Logger> logger_;               ///< 日志记录器
    std::unique_ptr<ThreadPool> threadPool_;       ///< 线程池
    std::unique_ptr<ConnectionManager> connMgr_;   ///< 连接管理器
    
    int listenFd_{-1};                             ///< 监听套接字
    int epollFd_{-1};                              ///< epoll文件描述符
    sockaddr_in serverAddr_{};                     ///< 服务器地址
    
    std::atomic<bool> running_{false};             ///< 运行状态
    std::atomic<bool> shouldStop_{false};          ///< 停止标志
    
    // 统计信息
    std::atomic<uint64_t> totalConnections_{0};    ///< 总连接数
    std::atomic<uint64_t> activeConnections_{0};   ///< 活跃连接数
    std::atomic<uint64_t> totalRequests_{0};       ///< 总请求数
    
    std::chrono::steady_clock::time_point startTime_; ///< 启动时间
    
    static std::atomic<bool> signalReceived_;       ///< 信号接收标志
    static int signalPipe_[2];                     ///< 信号管道
};

} // namespace webserver