#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

namespace webserver {

/**
 * @brief 套接字工具类
 * 
 * 提供常用的套接字操作函数，封装系统调用并提供异常安全的接口
 */
class SocketUtils {
public:
    /**
     * @brief 设置套接字为非阻塞模式
     * @param fd 文件描述符
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setNonBlocking(int fd);
    
    /**
     * @brief 设置地址重用选项
     * @param fd 文件描述符
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setReuseAddr(int fd);
    
    /**
     * @brief 设置端口重用选项
     * @param fd 文件描述符
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setReusePort(int fd);
    
    /**
     * @brief 设置TCP_NODELAY选项
     * @param fd 文件描述符
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setTcpNoDelay(int fd);
    
    /**
     * @brief 设置Keep-Alive选项
     * @param fd 文件描述符
     * @param keepIdle 开始发送keep-alive探测前的空闲时间
     * @param keepInterval 探测间隔
     * @param keepCount 探测次数
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setKeepAlive(int fd, int keepIdle = 600, int keepInterval = 30, int keepCount = 3);
    
    /**
     * @brief 设置接收缓冲区大小
     * @param fd 文件描述符
     * @param size 缓冲区大小
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setReceiveBuffer(int fd, int size);
    
    /**
     * @brief 设置发送缓冲区大小
     * @param fd 文件描述符
     * @param size 缓冲区大小
     * @throws std::runtime_error 设置失败时抛出异常
     */
    static void setSendBuffer(int fd, int size);
    
    /**
     * @brief 获取套接字错误
     * @param fd 文件描述符
     * @return 错误码
     */
    static int getSocketError(int fd);
    
    /**
     * @brief 将sockaddr_in转换为字符串
     * @param addr 地址结构
     * @return 地址字符串 (格式: "IP:PORT")
     */
    static std::string addrToString(const sockaddr_in& addr);
    
    /**
     * @brief 获取本地地址
     * @param fd 文件描述符
     * @return 本地地址
     * @throws std::runtime_error 获取失败时抛出异常
     */
    static sockaddr_in getLocalAddr(int fd);
    
    /**
     * @brief 获取对端地址
     * @param fd 文件描述符
     * @return 对端地址
     * @throws std::runtime_error 获取失败时抛出异常
     */
    static sockaddr_in getPeerAddr(int fd);
    
    /**
     * @brief 安全关闭套接字
     * @param fd 文件描述符
     */
    static void closeSocket(int fd) noexcept;

private:
    SocketUtils() = delete; // 工具类，禁止实例化
};

} // namespace webserver