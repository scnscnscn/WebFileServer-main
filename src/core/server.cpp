#include "server.h"
#include "../utils/socket_utils.h"
#include "../event/event_factory.h"

#include <sys/socket.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace webserver {

// 静态成员初始化
std::atomic<bool> WebServer::signalReceived_{false};
int WebServer::signalPipe_[2] = {-1, -1};

WebServer::WebServer(const ServerConfig& config) 
    : config_(config)
    , logger_(std::make_unique<Logger>(config.logLevel))
    , startTime_(std::chrono::steady_clock::now()) {
    
    logger_->info("Initializing WebServer with config: port={}, threads={}", 
                  config_.port, config_.threadCount);
    
    try {
        // 初始化线程池
        threadPool_ = std::make_unique<ThreadPool>(config_.threadCount);
        
        // 初始化连接管理器
        connMgr_ = std::make_unique<ConnectionManager>(config_.maxConnections);
        
        // 设置信号处理
        setupSignalHandling();
        
        logger_->info("WebServer initialized successfully");
        
    } catch (const std::exception& e) {
        logger_->error("Failed to initialize WebServer: {}", e.what());
        throw;
    }
}

WebServer::~WebServer() noexcept {
    try {
        stop(true);
        
        if (listenFd_ >= 0) {
            close(listenFd_);
        }
        if (epollFd_ >= 0) {
            close(epollFd_);
        }
        if (signalPipe_[0] >= 0) {
            close(signalPipe_[0]);
        }
        if (signalPipe_[1] >= 0) {
            close(signalPipe_[1]);
        }
        
        logger_->info("WebServer destroyed");
        
    } catch (...) {
        // 析构函数不应该抛出异常
    }
}

void WebServer::start() {
    if (running_.load()) {
        throw std::runtime_error("Server is already running");
    }
    
    logger_->info("Starting WebServer on port {}", config_.port);
    
    try {
        initializeListenSocket();
        initializeEpoll();
        
        running_.store(true);
        shouldStop_.store(false);
        
        logger_->info("WebServer started successfully, listening on port {}", config_.port);
        
        // 进入主事件循环
        eventLoop();
        
    } catch (const std::exception& e) {
        running_.store(false);
        logger_->error("Failed to start server: {}", e.what());
        throw;
    }
}

void WebServer::stop(bool graceful) noexcept {
    if (!running_.load()) {
        return;
    }
    
    logger_->info("Stopping WebServer (graceful={})", graceful);
    
    shouldStop_.store(true);
    
    if (graceful) {
        // 优雅关闭：等待现有连接处理完成
        auto timeout = std::chrono::seconds(config_.shutdownTimeout);
        auto start = std::chrono::steady_clock::now();
        
        while (activeConnections_.load() > 0 && 
               std::chrono::steady_clock::now() - start < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    running_.store(false);
    logger_->info("WebServer stopped");
}

std::string WebServer::getStats() const {
    auto uptime = std::chrono::steady_clock::now() - startTime_;
    auto uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
    
    return fmt::format(
        "Server Stats:\n"
        "  Uptime: {} seconds\n"
        "  Total Connections: {}\n"
        "  Active Connections: {}\n"
        "  Total Requests: {}\n"
        "  Thread Pool Size: {}",
        uptimeSeconds,
        totalConnections_.load(),
        activeConnections_.load(),
        totalRequests_.load(),
        config_.threadCount
    );
}

void WebServer::initializeListenSocket() {
    // 创建套接字
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    
    try {
        // 设置套接字选项
        SocketUtils::setReuseAddr(listenFd_);
        SocketUtils::setReusePort(listenFd_);
        SocketUtils::setNonBlocking(listenFd_);
        
        // 绑定地址
        serverAddr_.sin_family = AF_INET;
        serverAddr_.sin_port = htons(config_.port);
        serverAddr_.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(listenFd_, reinterpret_cast<sockaddr*>(&serverAddr_), sizeof(serverAddr_)) < 0) {
            throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
        }
        
        // 开始监听
        if (listen(listenFd_, config_.backlog) < 0) {
            throw std::runtime_error("Failed to listen: " + std::string(strerror(errno)));
        }
        
        logger_->info("Listen socket created and bound to port {}", config_.port);
        
    } catch (...) {
        close(listenFd_);
        listenFd_ = -1;
        throw;
    }
}

void WebServer::initializeEpoll() {
    epollFd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epollFd_ < 0) {
        throw std::runtime_error("Failed to create epoll: " + std::string(strerror(errno)));
    }
    
    try {
        // 添加监听套接字到epoll
        epoll_event event{};
        event.events = EPOLLIN | EPOLLET;  // 边缘触发
        event.data.fd = listenFd_;
        
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &event) < 0) {
            throw std::runtime_error("Failed to add listen socket to epoll: " + 
                                   std::string(strerror(errno)));
        }
        
        // 添加信号管道到epoll
        if (signalPipe_[0] >= 0) {
            event.events = EPOLLIN;
            event.data.fd = signalPipe_[0];
            
            if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, signalPipe_[0], &event) < 0) {
                throw std::runtime_error("Failed to add signal pipe to epoll: " + 
                                       std::string(strerror(errno)));
            }
        }
        
        logger_->info("Epoll initialized successfully");
        
    } catch (...) {
        close(epollFd_);
        epollFd_ = -1;
        throw;
    }
}

void WebServer::setupSignalHandling() {
    // 创建信号管道
    if (pipe2(signalPipe_, O_CLOEXEC | O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to create signal pipe: " + std::string(strerror(errno)));
    }
    
    // 设置信号处理函数
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGINT, &sa, nullptr) < 0 ||
        sigaction(SIGTERM, &sa, nullptr) < 0) {
        throw std::runtime_error("Failed to setup signal handling: " + std::string(strerror(errno)));
    }
    
    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);
    
    logger_->info("Signal handling setup completed");
}

void WebServer::eventLoop() {
    const int maxEvents = 1024;
    std::vector<epoll_event> events(maxEvents);
    
    logger_->info("Entering main event loop");
    
    while (running_.load() && !shouldStop_.load()) {
        try {
            int numEvents = epoll_wait(epollFd_, events.data(), maxEvents, 1000); // 1秒超时
            
            if (numEvents < 0) {
                if (errno == EINTR) {
                    continue; // 被信号中断，继续循环
                }
                logger_->error("epoll_wait failed: {}", strerror(errno));
                break;
            }
            
            if (numEvents == 0) {
                // 超时，检查是否需要清理连接
                connMgr_->cleanupIdleConnections();
                continue;
            }
            
            // 处理所有就绪事件
            for (int i = 0; i < numEvents; ++i) {
                int fd = events[i].data.fd;
                uint32_t eventMask = events[i].events;
                
                if (fd == listenFd_) {
                    handleNewConnection();
                } else if (fd == signalPipe_[0]) {
                    // 处理信号
                    char buffer[256];
                    while (read(signalPipe_[0], buffer, sizeof(buffer)) > 0) {
                        // 消费信号数据
                    }
                    if (signalReceived_.load()) {
                        logger_->info("Received shutdown signal");
                        shouldStop_.store(true);
                    }
                } else {
                    handleClientEvent(fd, eventMask);
                }
            }
            
        } catch (const std::exception& e) {
            logger_->error("Exception in event loop: {}", e.what());
            // 继续运行，不因单个异常而停止服务器
        }
    }
    
    logger_->info("Exiting main event loop");
}

void WebServer::handleNewConnection() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientFd = accept4(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), 
                              &clientLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接
            }
            logger_->error("Failed to accept connection: {}", strerror(errno));
            break;
        }
        
        try {
            // 检查连接数限制
            if (activeConnections_.load() >= config_.maxConnections) {
                logger_->warn("Connection limit reached, rejecting new connection");
                close(clientFd);
                continue;
            }
            
            // 创建连接对象
            auto connection = connMgr_->createConnection(clientFd, clientAddr);
            
            // 添加到epoll监听
            epoll_event event{};
            event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            event.data.fd = clientFd;
            
            if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &event) < 0) {
                logger_->error("Failed to add client socket to epoll: {}", strerror(errno));
                connMgr_->removeConnection(clientFd);
                continue;
            }
            
            totalConnections_.fetch_add(1);
            activeConnections_.fetch_add(1);
            
            logger_->debug("New connection accepted: fd={}, total={}", 
                          clientFd, activeConnections_.load());
            
        } catch (const std::exception& e) {
            logger_->error("Failed to handle new connection: {}", e.what());
            close(clientFd);
        }
    }
}

void WebServer::handleClientEvent(int fd, uint32_t events) {
    try {
        auto connection = connMgr_->getConnection(fd);
        if (!connection) {
            logger_->warn("Received event for unknown connection: fd={}", fd);
            epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
            return;
        }
        
        // 创建事件处理器并提交到线程池
        if (events & EPOLLIN) {
            auto handler = EventFactory::createReceiveHandler(fd, epollFd_);
            threadPool_->submit([handler = std::move(handler)]() {
                handler->process();
            });
            totalRequests_.fetch_add(1);
        }
        
        if (events & EPOLLOUT) {
            auto handler = EventFactory::createSendHandler(fd, epollFd_);
            threadPool_->submit([handler = std::move(handler)]() {
                handler->process();
            });
        }
        
        if (events & (EPOLLHUP | EPOLLERR)) {
            logger_->debug("Connection closed or error: fd={}", fd);
            connMgr_->removeConnection(fd);
            activeConnections_.fetch_sub(1);
        }
        
    } catch (const std::exception& e) {
        logger_->error("Failed to handle client event: {}", e.what());
        connMgr_->removeConnection(fd);
        activeConnections_.fetch_sub(1);
    }
}

void WebServer::signalHandler(int signum) {
    signalReceived_.store(true);
    
    // 通过管道通知主线程
    if (signalPipe_[1] >= 0) {
        char sig = static_cast<char>(signum);
        ssize_t result = write(signalPipe_[1], &sig, 1);
        (void)result; // 避免未使用变量警告
    }
}

} // namespace webserver