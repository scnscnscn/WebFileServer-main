#pragma once

#include <memory>
#include <chrono>
#include <atomic>
#include <string>
#include <netinet/in.h>

namespace webserver {

/**
 * @brief 连接状态枚举
 */
enum class ConnectionState {
    CONNECTING,     ///< 正在连接
    CONNECTED,      ///< 已连接
    READING,        ///< 正在读取
    WRITING,        ///< 正在写入
    CLOSING,        ///< 正在关闭
    CLOSED          ///< 已关闭
};

/**
 * @brief HTTP连接类
 * 
 * 管理单个HTTP连接的生命周期，包括状态跟踪、超时管理等
 */
class Connection {
public:
    /**
     * @brief 构造函数
     * @param fd 文件描述符
     * @param addr 客户端地址
     */
    Connection(int fd, const sockaddr_in& addr);
    
    /**
     * @brief 析构函数
     */
    ~Connection() noexcept;
    
    // 禁用拷贝构造和拷贝赋值
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    
    // 启用移动构造和移动赋值
    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;
    
    /**
     * @brief 获取文件描述符
     * @return 文件描述符
     */
    int getFd() const noexcept { return fd_; }
    
    /**
     * @brief 获取客户端地址
     * @return 客户端地址
     */
    const sockaddr_in& getClientAddr() const noexcept { return clientAddr_; }
    
    /**
     * @brief 获取客户端地址字符串
     * @return 地址字符串
     */
    std::string getClientAddrString() const;
    
    /**
     * @brief 获取连接状态
     * @return 连接状态
     */
    ConnectionState getState() const noexcept { return state_.load(); }
    
    /**
     * @brief 设置连接状态
     * @param state 新状态
     */
    void setState(ConnectionState state) noexcept { state_.store(state); }
    
    /**
     * @brief 更新最后活动时间
     */
    void updateLastActivity() noexcept {
        lastActivity_ = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief 获取最后活动时间
     * @return 最后活动时间点
     */
    std::chrono::steady_clock::time_point getLastActivity() const noexcept {
        return lastActivity_;
    }
    
    /**
     * @brief 检查连接是否超时
     * @param timeout 超时时间
     * @return true表示已超时
     */
    bool isTimeout(std::chrono::seconds timeout) const noexcept {
        auto now = std::chrono::steady_clock::now();
        return (now - lastActivity_) > timeout;
    }
    
    /**
     * @brief 获取连接创建时间
     * @return 创建时间点
     */
    std::chrono::steady_clock::time_point getCreateTime() const noexcept {
        return createTime_;
    }
    
    /**
     * @brief 获取连接持续时间
     * @return 持续时间（秒）
     */
    std::chrono::seconds getDuration() const noexcept {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - createTime_);
    }
    
    /**
     * @brief 增加请求计数
     */
    void incrementRequestCount() noexcept { requestCount_.fetch_add(1); }
    
    /**
     * @brief 获取请求计数
     * @return 请求总数
     */
    uint64_t getRequestCount() const noexcept { return requestCount_.load(); }
    
    /**
     * @brief 关闭连接
     */
    void close() noexcept;
    
    /**
     * @brief 检查连接是否已关闭
     * @return true表示已关闭
     */
    bool isClosed() const noexcept { 
        return state_.load() == ConnectionState::CLOSED || fd_ < 0; 
    }

private:
    int fd_;                                                    ///< 文件描述符
    sockaddr_in clientAddr_;                                   ///< 客户端地址
    std::atomic<ConnectionState> state_{ConnectionState::CONNECTED}; ///< 连接状态
    
    std::chrono::steady_clock::time_point createTime_;        ///< 创建时间
    std::chrono::steady_clock::time_point lastActivity_;      ///< 最后活动时间
    
    std::atomic<uint64_t> requestCount_{0};                   ///< 请求计数
};

} // namespace webserver