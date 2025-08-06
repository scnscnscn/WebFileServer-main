#include "utils.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <unistd.h>

namespace webserver {

std::string createLogPrefix(LogLevel level) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // 获取微秒
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration) % 1000000;
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") 
        << '.' << std::setfill('0') << std::setw(6) << microseconds.count()
        << ' ' << std::put_time(&tm, "%Y-%m-%d") << " [";
    
    switch (level) {
        case LogLevel::INFO:    oss << "INFO"; break;
        case LogLevel::ERROR:   oss << "ERROR"; break;
        case LogLevel::DEBUG:   oss << "DEBUG"; break;
        case LogLevel::WARNING: oss << "WARN"; break;
    }
    
    oss << "]: ";
    return oss.str();
}

void addEpollFd(int epollFd, int fd, bool edgeTrigger, bool oneshot) {
    epoll_event event{};
    event.data.fd = fd;
    event.events = EPOLLIN;
    
    if (edgeTrigger) {
        event.events |= EPOLLET;
    }
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        throw std::runtime_error("Failed to add fd to epoll: " + std::string(strerror(errno)));
    }
}

void modifyEpollFd(int epollFd, int fd, bool edgeTrigger, bool oneshot, bool enableWrite) {
    epoll_event event{};
    event.data.fd = fd;
    event.events = EPOLLIN;
    
    if (edgeTrigger) {
        event.events |= EPOLLET;
    }
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    if (enableWrite) {
        event.events |= EPOLLOUT;
    }
    
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw std::runtime_error("Failed to modify fd in epoll: " + std::string(strerror(errno)));
    }
}

void removeEpollFd(int epollFd, int fd) {
    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw std::runtime_error("Failed to remove fd from epoll: " + std::string(strerror(errno)));
    }
}

void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        throw std::runtime_error("Failed to get fd flags: " + std::string(strerror(errno)));
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set fd non-blocking: " + std::string(strerror(errno)));
    }
}

} // namespace webserver