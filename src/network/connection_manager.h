#pragma once

#include "connection.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>

namespace webserver {

/**
 * @brief 连接管理器
 * 
 * 负责管理所有活跃连接，提供连接的创建、查找、删除和超时清理功能
 */
class ConnectionManager {
public:
    /**
     * @brief 构造函数
     * @param maxConnections 最大连接数
     */
    explicit ConnectionManager(size_t maxConnections = 10000);
    
    /**
     * @brief 析构函数
     */
    ~ConnectionManager() noexcept;
    
    // 禁用拷贝构造和拷贝赋值
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
    
    /**
     * @brief 创建新连接
     * @param fd 文件描述符
     * @param addr 客户端地址
     * @return 连接对象的共享指针
     * @throws std::runtime_error 连接数超过限制时抛出异常
     */
    std::shared_ptr<Connection> createConnection(int fd, const sockaddr_in& addr);
    
    /**
     * @brief 获取连接
     * @param fd 文件描述符
     * @return 连接对象的共享指针，不存在时返回nullptr
     */
    std::shared_ptr<Connection> getConnection(int fd);
    
    /**
     * @brief 删除连接
     * @param fd 文件描述符
     * @return true表示删除成功
     */
    bool removeConnection(int fd);
    
    /**
     * @brief 获取当前连接数
     * @return 连接数
     */
    size_t getConnectionCount() const;
    
    /**
     * @brief 获取最大连接数
     * @return 最大连接数
     */
    size_t getMaxConnections() const noexcept { return maxConnections_; }
    
    /**
     * @brief 清理空闲连接
     * @param timeout 超时时间，默认30秒
     * @return 清理的连接数
     */
    size_t cleanupIdleConnections(std::chrono::seconds timeout = std::chrono::seconds(30));
    
    /**
     * @brief 获取所有连接的统计信息
     * @return 统计信息字符串
     */
    std::string getStats() const;
    
    /**
     * @brief 关闭所有连接
     */
    void closeAllConnections();

private:
    mutable std::mutex mutex_;                                  ///< 互斥锁
    std::unordered_map<int, std::shared_ptr<Connection>> connections_; ///< 连接映射
    size_t maxConnections_;                                    ///< 最大连接数
    
    // 统计信息
    std::atomic<uint64_t> totalConnections_{0};               ///< 总连接数
    std::atomic<uint64_t> totalRequests_{0};                  ///< 总请求数
};

} // namespace webserver