#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace webserver {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * @brief 现代化的异步日志记录器
 * 
 * 特性：
 * - 异步写入，不阻塞主线程
 * - 线程安全
 * - 支持格式化输出
 * - 自动日志轮转
 * - 异常安全
 */
class Logger {
public:
    /**
     * @brief 构造函数
     * @param level 日志级别
     * @param filename 日志文件名，空表示输出到控制台
     * @param maxFileSize 最大文件大小，超过后轮转
     */
    explicit Logger(LogLevel level = LogLevel::INFO, 
                   const std::string& filename = "",
                   size_t maxFileSize = 10 * 1024 * 1024);
    
    /**
     * @brief 析构函数
     */
    ~Logger() noexcept;
    
    // 禁用拷贝构造和拷贝赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief 记录调试信息
     * @tparam Args 参数类型
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void debug(const std::string& format, Args&&... args);
    
    /**
     * @brief 记录一般信息
     * @tparam Args 参数类型
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void info(const std::string& format, Args&&... args);
    
    /**
     * @brief 记录警告信息
     * @tparam Args 参数类型
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void warn(const std::string& format, Args&&... args);
    
    /**
     * @brief 记录错误信息
     * @tparam Args 参数类型
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void error(const std::string& format, Args&&... args);
    
    /**
     * @brief 设置日志级别
     * @param level 新的日志级别
     */
    void setLevel(LogLevel level) noexcept { level_ = level; }
    
    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    LogLevel getLevel() const noexcept { return level_; }
    
    /**
     * @brief 刷新日志缓冲区
     */
    void flush();

private:
    /**
     * @brief 日志条目结构
     */
    struct LogEntry {
        LogLevel level;
        std::chrono::system_clock::time_point timestamp;
        std::thread::id threadId;
        std::string message;
        
        LogEntry(LogLevel lvl, std::string msg)
            : level(lvl), timestamp(std::chrono::system_clock::now()),
              threadId(std::this_thread::get_id()), message(std::move(msg)) {}
    };
    
    /**
     * @brief 记录日志的内部实现
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief 格式化日志消息
     * @tparam Args 参数类型
     * @param format 格式字符串
     * @param args 参数
     * @return 格式化后的字符串
     */
    template<typename... Args>
    std::string formatMessage(const std::string& format, Args&&... args);
    
    /**
     * @brief 后台写入线程函数
     */
    void writerThread();
    
    /**
     * @brief 格式化日志条目
     * @param entry 日志条目
     * @return 格式化后的字符串
     */
    std::string formatLogEntry(const LogEntry& entry);
    
    /**
     * @brief 检查是否需要轮转日志文件
     */
    void checkRotation();
    
    /**
     * @brief 轮转日志文件
     */
    void rotateLogFile();
    
    /**
     * @brief 获取日志级别字符串
     * @param level 日志级别
     * @return 级别字符串
     */
    static const char* getLevelString(LogLevel level);

private:
    std::atomic<LogLevel> level_;                        ///< 当前日志级别
    std::string filename_;                               ///< 日志文件名
    size_t maxFileSize_;                                ///< 最大文件大小
    
    std::queue<LogEntry> logQueue_;                     ///< 日志队列
    std::mutex queueMutex_;                             ///< 队列互斥锁
    std::condition_variable condition_;                  ///< 条件变量
    
    std::unique_ptr<std::ofstream> fileStream_;         ///< 文件输出流
    std::atomic<bool> shutdown_{false};                 ///< 关闭标志
    std::thread writerThread_;                          ///< 写入线程
    
    size_t currentFileSize_{0};                         ///< 当前文件大小
};

// 模板方法实现
template<typename... Args>
void Logger::debug(const std::string& format, Args&&... args) {
    if (level_.load() <= LogLevel::DEBUG) {
        log(LogLevel::DEBUG, formatMessage(format, std::forward<Args>(args)...));
    }
}

template<typename... Args>
void Logger::info(const std::string& format, Args&&... args) {
    if (level_.load() <= LogLevel::INFO) {
        log(LogLevel::INFO, formatMessage(format, std::forward<Args>(args)...));
    }
}

template<typename... Args>
void Logger::warn(const std::string& format, Args&&... args) {
    if (level_.load() <= LogLevel::WARN) {
        log(LogLevel::WARN, formatMessage(format, std::forward<Args>(args)...));
    }
}

template<typename... Args>
void Logger::error(const std::string& format, Args&&... args) {
    if (level_.load() <= LogLevel::ERROR) {
        log(LogLevel::ERROR, formatMessage(format, std::forward<Args>(args)...));
    }
}

template<typename... Args>
std::string Logger::formatMessage(const std::string& format, Args&&... args) {
    if constexpr (sizeof...(args) == 0) {
        return format;
    } else {
        // 简单的格式化实现，实际项目中可以使用fmt库
        std::ostringstream oss;
        size_t pos = 0;
        size_t argIndex = 0;
        
        auto formatArg = [&oss](auto&& arg) {
            oss << arg;
        };
        
        while (pos < format.length()) {
            size_t nextPos = format.find("{}", pos);
            if (nextPos == std::string::npos) {
                oss << format.substr(pos);
                break;
            }
            
            oss << format.substr(pos, nextPos - pos);
            
            if (argIndex < sizeof...(args)) {
                // 使用fold expression (C++17)
                size_t currentIndex = 0;
                ((currentIndex++ == argIndex ? formatArg(args) : void()), ...);
                argIndex++;
            }
            
            pos = nextPos + 2;
        }
        
        return oss.str();
    }
}

} // namespace webserver