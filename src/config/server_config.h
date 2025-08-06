#pragma once

#include <string>
#include <chrono>

namespace webserver {

/**
 * @brief 服务器配置结构体
 * 
 * 集中管理所有服务器配置参数，支持从配置文件或命令行参数加载
 */
struct ServerConfig {
    // 网络配置
    int port{8888};                                    ///< 监听端口
    std::string bindAddress{"0.0.0.0"};              ///< 绑定地址
    int backlog{1024};                                ///< 监听队列长度
    int maxConnections{10000};                        ///< 最大连接数
    
    // 线程池配置
    int threadCount{std::thread::hardware_concurrency()}; ///< 工作线程数
    int maxQueueSize{10000};                          ///< 任务队列最大长度
    
    // 超时配置
    std::chrono::seconds connectionTimeout{30};       ///< 连接超时时间
    std::chrono::seconds keepAliveTimeout{60};        ///< Keep-Alive超时时间
    std::chrono::seconds shutdownTimeout{10};         ///< 优雅关闭超时时间
    
    // 文件配置
    std::string documentRoot{"./filedir"};            ///< 文档根目录
    std::string templateDir{"./html"};                ///< 模板目录
    size_t maxFileSize{100 * 1024 * 1024};           ///< 最大文件大小 (100MB)
    size_t bufferSize{8192};                          ///< 缓冲区大小
    
    // 日志配置
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };
    LogLevel logLevel{LogLevel::INFO};                ///< 日志级别
    std::string logFile;                              ///< 日志文件路径 (空表示控制台输出)
    
    // 性能配置
    bool enableSendfile{true};                        ///< 启用sendfile零拷贝
    bool enableKeepalive{true};                       ///< 启用HTTP Keep-Alive
    bool enableGzip{false};                           ///< 启用Gzip压缩
    
    /**
     * @brief 从配置文件加载配置
     * @param configFile 配置文件路径
     * @return 配置对象
     * @throws std::runtime_error 加载失败时抛出异常
     */
    static ServerConfig loadFromFile(const std::string& configFile);
    
    /**
     * @brief 从命令行参数加载配置
     * @param argc 参数个数
     * @param argv 参数数组
     * @return 配置对象
     */
    static ServerConfig loadFromArgs(int argc, char* argv[]);
    
    /**
     * @brief 验证配置的有效性
     * @throws std::invalid_argument 配置无效时抛出异常
     */
    void validate() const;
    
    /**
     * @brief 获取配置的字符串表示
     * @return 配置信息字符串
     */
    std::string toString() const;
};

} // namespace webserver