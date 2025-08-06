#include "src/core/server.h"
#include "src/config/server_config.h"
#include <iostream>
#include <csignal>
#include <memory>

using namespace webserver;

// 全局服务器实例，用于信号处理
std::unique_ptr<WebServer> g_server;

/**
 * @brief 信号处理函数
 * @param signum 信号编号
 */
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down gracefully..." << std::endl;
    if (g_server) {
        g_server->stop(true);
    }
}

/**
 * @brief 打印使用帮助
 * @param programName 程序名称
 */
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -p, --port <port>        Listen port (default: 8888)\n"
              << "  -t, --threads <count>    Thread pool size (default: CPU cores)\n"
              << "  -d, --document-root <path>  Document root directory (default: ./filedir)\n"
              << "  -l, --log-level <level>  Log level (debug|info|warn|error, default: info)\n"
              << "  -f, --log-file <file>    Log file path (default: console output)\n"
              << "  -c, --config <file>      Configuration file path\n"
              << "  -h, --help               Show this help message\n"
              << std::endl;
}

/**
 * @brief 程序入口点
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 退出码
 */
int main(int argc, char* argv[]) {
    try {
        // 解析命令行参数或配置文件
        ServerConfig config;
        
        // 简单的命令行参数解析
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "--help") {
                printUsage(argv[0]);
                return 0;
            } else if (arg == "-p" || arg == "--port") {
                if (i + 1 < argc) {
                    config.port = std::stoi(argv[++i]);
                } else {
                    std::cerr << "Error: " << arg << " requires a value" << std::endl;
                    return 1;
                }
            } else if (arg == "-t" || arg == "--threads") {
                if (i + 1 < argc) {
                    config.threadCount = std::stoi(argv[++i]);
                } else {
                    std::cerr << "Error: " << arg << " requires a value" << std::endl;
                    return 1;
                }
            } else if (arg == "-d" || arg == "--document-root") {
                if (i + 1 < argc) {
                    config.documentRoot = argv[++i];
                } else {
                    std::cerr << "Error: " << arg << " requires a value" << std::endl;
                    return 1;
                }
            } else if (arg == "-l" || arg == "--log-level") {
                if (i + 1 < argc) {
                    std::string level = argv[++i];
                    if (level == "debug") config.logLevel = ServerConfig::LogLevel::DEBUG;
                    else if (level == "info") config.logLevel = ServerConfig::LogLevel::INFO;
                    else if (level == "warn") config.logLevel = ServerConfig::LogLevel::WARN;
                    else if (level == "error") config.logLevel = ServerConfig::LogLevel::ERROR;
                    else {
                        std::cerr << "Error: Invalid log level: " << level << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires a value" << std::endl;
                    return 1;
                }
            } else if (arg == "-f" || arg == "--log-file") {
                if (i + 1 < argc) {
                    config.logFile = argv[++i];
                } else {
                    std::cerr << "Error: " << arg << " requires a value" << std::endl;
                    return 1;
                }
            } else if (arg == "-c" || arg == "--config") {
                if (i + 1 < argc) {
                    try {
                        config = ServerConfig::loadFromFile(argv[++i]);
                    } catch (const std::exception& e) {
                        std::cerr << "Error loading config file: " << e.what() << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires a value" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: Unknown option: " << arg << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
        
        // 验证配置
        try {
            config.validate();
        } catch (const std::exception& e) {
            std::cerr << "Configuration error: " << e.what() << std::endl;
            return 1;
        }
        
        // 打印配置信息
        std::cout << "Starting WebFileServer with configuration:\n"
                  << config.toString() << std::endl;
        
        // 设置信号处理
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // 创建并启动服务器
        g_server = std::make_unique<WebServer>(config);
        
        std::cout << "WebFileServer starting..." << std::endl;
        g_server->start();
        
        std::cout << "WebFileServer stopped." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}