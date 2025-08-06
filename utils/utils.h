#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <stdexcept>

#include <sys/time.h>
#include <sys/epoll.h>
#include <fcntl.h>

namespace webserver {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    INFO,
    ERROR,
    DEBUG,
    WARNING
};

/**
 * @brief 生成带时间戳的日志前缀
 * @param level 日志级别
 * @return 格式化的日志前缀字符串
 * 
 * 格式: "HH:MM:SS.microsec YYYY-MM-DD [LEVEL]: "
 */
std::string createLogPrefix(LogLevel level);

/**
 * @brief 向epoll实例添加文件描述符
 * @param epollFd epoll文件描述符
 * @param fd 要添加的文件描述符
 * @param edgeTrigger 是否使用边缘触发模式
 * @param oneshot 是否使用EPOLLONESHOT模式
 * @throws std::runtime_error 添加失败时抛出异常
 */
void addEpollFd(int epollFd, int fd, bool edgeTrigger = false, bool oneshot = false);

/**
 * @brief 修改epoll中文件描述符的监听事件
 * @param epollFd epoll文件描述符
 * @param fd 要修改的文件描述符
 * @param edgeTrigger 是否使用边缘触发模式
 * @param oneshot 是否重置EPOLLONESHOT
 * @param enableWrite 是否启用写事件监听
 * @throws std::runtime_error 修改失败时抛出异常
 */
void modifyEpollFd(int epollFd, int fd, bool edgeTrigger = false, 
                   bool oneshot = false, bool enableWrite = false);

/**
 * @brief 从epoll中删除文件描述符
 * @param epollFd epoll文件描述符
 * @param fd 要删除的文件描述符
 * @throws std::runtime_error 删除失败时抛出异常
 */
void removeEpollFd(int epollFd, int fd);

/**
 * @brief 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
 * @throws std::runtime_error 设置失败时抛出异常
 */
void setNonBlocking(int fd);

/**
 * @brief RAII风格的文件描述符管理器
 */
class FileDescriptor {
public:
    explicit FileDescriptor(int fd = -1) noexcept : fd_(fd) {}
    
    ~FileDescriptor() noexcept {
        close();
    }
    
    // 禁用拷贝构造和拷贝赋值
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    
    // 启用移动构造和移动赋值
    FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }
    
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }
    
    int get() const noexcept { return fd_; }
    bool valid() const noexcept { return fd_ >= 0; }
    
    int release() noexcept {
        int temp = fd_;
        fd_ = -1;
        return temp;
    }
    
    void reset(int fd = -1) noexcept {
        close();
        fd_ = fd;
    }
    
private:
    void close() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    int fd_;
};

} // namespace webserver