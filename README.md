# Modern C++ Web File Server

一个使用现代C++17标准实现的高性能Web文件服务器，支持文件上传、下载、删除和列表展示功能。

## 🚀 特性

### 核心功能
- **文件管理**: 支持文件上传、下载、删除和列表展示
- **HTTP协议**: 完整的HTTP/1.1协议支持
- **多线程**: 基于线程池的并发处理
- **事件驱动**: 使用epoll的高性能I/O多路复用
- **零拷贝**: 使用sendfile实现高效文件传输

### 现代C++特性
- **C++17标准**: 使用现代C++语言特性
- **RAII设计**: 自动资源管理，异常安全
- **智能指针**: 内存安全，避免内存泄漏
- **异步日志**: 高性能的异步日志系统
- **配置化**: 支持配置文件和命令行参数

### 性能优化
- **Reactor模式**: 事件驱动的网络编程模式
- **连接池**: 高效的连接管理
- **非阻塞I/O**: 避免线程阻塞
- **边缘触发**: epoll边缘触发模式
- **优雅关闭**: 支持优雅的服务器关闭

## 📋 系统要求

- **操作系统**: Linux (内核版本 >= 2.6)
- **编译器**: GCC >= 7.0 或 Clang >= 5.0 (支持C++17)
- **CMake**: >= 3.12
- **内存**: 建议 >= 512MB
- **磁盘**: 根据文件存储需求

## 🛠️ 编译安装

### 使用CMake编译

```bash
# 克隆项目
git clone https://github.com/your-repo/WebFileServer.git
cd WebFileServer

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 安装（可选）
sudo make install
```

### 使用传统Makefile编译

```bash
# 直接编译
make

# 清理
make clean
```

## 🚀 快速开始

### 基本使用

```bash
# 使用默认配置启动服务器
./WebFileServer

# 指定端口启动
./WebFileServer --port 9000

# 指定线程数
./WebFileServer --threads 8

# 指定文档根目录
./WebFileServer --document-root /path/to/files
```

### 命令行参数

```bash
Usage: WebFileServer [options]
Options:
  -p, --port <port>        监听端口 (默认: 8888)
  -t, --threads <count>    线程池大小 (默认: CPU核心数)
  -d, --document-root <path>  文档根目录 (默认: ./filedir)
  -l, --log-level <level>  日志级别 (debug|info|warn|error, 默认: info)
  -f, --log-file <file>    日志文件路径 (默认: 控制台输出)
  -c, --config <file>      配置文件路径
  -h, --help               显示帮助信息
```

### 配置文件

创建 `server.conf` 配置文件：

```ini
# 网络配置
port = 8888
bind_address = 0.0.0.0
max_connections = 10000

# 线程配置
thread_count = 8
max_queue_size = 10000

# 文件配置
document_root = ./filedir
template_dir = ./html
max_file_size = 104857600  # 100MB

# 日志配置
log_level = info
log_file = /var/log/webserver.log

# 性能配置
enable_sendfile = true
enable_keepalive = true
```

## 📁 项目结构

```
WebFileServer/
├── src/                    # 源代码目录
│   ├── core/              # 核心服务器类
│   ├── config/            # 配置管理
│   ├── threadpool/        # 线程池实现
│   ├── utils/             # 工具类
│   ├── network/           # 网络连接管理
│   ├── http/              # HTTP协议处理
│   ├── event/             # 事件处理
│   └── file/              # 文件操作
├── html/                  # HTML模板
├── filedir/               # 文件存储目录
├── config/                # 配置文件示例
├── tests/                 # 单元测试
├── docs/                  # 文档
├── CMakeLists.txt         # CMake构建文件
├── Makefile              # 传统Makefile
└── README.md             # 项目说明
```

## 🔧 架构设计

### 整体架构

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   浏览器客户端   │────│  HTTP协议通信   │────│   Web服务器     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                       │
                       ┌───────────────────────────────┼───────────────────────────────┐
                       │                               │                               │
                ┌─────────────┐              ┌─────────────┐              ┌─────────────┐
                │  主线程     │              │  线程池     │              │  文件系统   │
                │ (epoll监听) │              │ (事件处理)  │              │ (文件操作)  │
                └─────────────┘              └─────────────┘              └─────────────┘
```

### 核心组件

1. **WebServer**: 服务器核心类，管理整个服务器生命周期
2. **ThreadPool**: 现代化线程池，支持任意可调用对象
3. **ConnectionManager**: 连接管理器，负责连接的创建、管理和清理
4. **HttpParser**: HTTP协议解析器，支持分块解析
5. **Logger**: 异步日志系统，高性能日志记录
6. **EventHandlers**: 事件处理器，处理各种网络事件

### 设计模式

- **Reactor模式**: 事件驱动的网络编程
- **RAII模式**: 资源自动管理
- **工厂模式**: 事件处理器创建
- **单例模式**: 全局配置管理
- **观察者模式**: 事件通知机制

## 📊 性能特性

### 并发性能
- **高并发**: 支持万级并发连接
- **低延迟**: 事件驱动，响应迅速
- **高吞吐**: 零拷贝文件传输
- **内存效率**: 智能指针管理，避免内存泄漏

### 优化技术
- **epoll边缘触发**: 减少系统调用
- **sendfile零拷贝**: 高效文件传输
- **连接复用**: HTTP Keep-Alive支持
- **异步日志**: 不阻塞主线程

## 🔒 安全特性

### 基础安全
- **路径验证**: 防止目录遍历攻击
- **文件类型检查**: 限制上传文件类型
- **大小限制**: 防止大文件攻击
- **连接限制**: 防止连接耗尽攻击

### 建议增强
- **HTTPS支持**: SSL/TLS加密传输
- **用户认证**: 基于令牌的认证
- **访问控制**: 基于角色的权限管理
- **审计日志**: 详细的操作记录

## 🧪 测试

### 单元测试

```bash
# 运行所有测试
make test

# 运行特定测试
./tests/test_http_parser
./tests/test_thread_pool
```

### 性能测试

```bash
# 使用ab进行压力测试
ab -n 10000 -c 100 http://localhost:8888/

# 使用wrk进行性能测试
wrk -t12 -c400 -d30s http://localhost:8888/
```

### 内存检查

```bash
# 使用valgrind检查内存泄漏
valgrind --leak-check=full ./WebFileServer

# 使用AddressSanitizer
g++ -fsanitize=address -g -o WebFileServer_debug src/*.cpp
./WebFileServer_debug
```

## 📈 监控和调试

### 日志分析

```bash
# 查看实时日志
tail -f /var/log/webserver.log

# 分析错误日志
grep "ERROR" /var/log/webserver.log

# 统计请求数
grep "INFO.*request" /var/log/webserver.log | wc -l
```

### 性能监控

```bash
# 查看进程状态
top -p $(pgrep WebFileServer)

# 查看网络连接
netstat -anp | grep :8888

# 查看文件描述符使用情况
lsof -p $(pgrep WebFileServer)
```

## 🛣️ 发展路线图

### 短期目标 (v1.1)
- [ ] HTTPS支持
- [ ] 配置热重载
- [ ] 更多HTTP方法支持
- [ ] 压缩传输支持

### 中期目标 (v1.5)
- [ ] 用户认证系统
- [ ] 文件权限管理
- [ ] RESTful API
- [ ] WebSocket支持

### 长期目标 (v2.0)
- [ ] HTTP/2支持
- [ ] 分布式部署
- [ ] 缓存系统
- [ ] 负载均衡

## 🤝 贡献指南

### 开发环境设置

```bash
# 安装开发依赖
sudo apt-get install build-essential cmake git

# 安装代码格式化工具
sudo apt-get install clang-format

# 安装静态分析工具
sudo apt-get install cppcheck clang-tidy
```

### 代码规范

- 使用C++17标准
- 遵循Google C++代码规范
- 使用clang-format格式化代码
- 编写单元测试
- 添加详细注释

### 提交流程

1. Fork项目
2. 创建特性分支
3. 编写代码和测试
4. 运行所有测试
5. 提交Pull Request

## 📄 许可证

本项目采用MIT许可证，详见 [LICENSE](LICENSE) 文件。

## 👥 作者

- **主要开发者**: [Your Name](mailto:your.email@example.com)
- **贡献者**: 查看 [CONTRIBUTORS.md](CONTRIBUTORS.md)

## 🙏 致谢

感谢以下项目和资源的启发：

- [TinyWebServer](https://github.com/qinguoyi/TinyWebServer)
- [muduo网络库](https://github.com/chenshuo/muduo)
- [nginx](https://nginx.org/)
- 《Linux高性能服务器编程》
- 《TCP/IP网络编程》

## 📞 支持

如果您遇到问题或有建议，请：

1. 查看 [FAQ](docs/FAQ.md)
2. 搜索 [Issues](https://github.com/your-repo/WebFileServer/issues)
3. 创建新的Issue
4. 发送邮件至 support@webserver.com

---

**WebFileServer** - 现代C++构建的高性能Web文件服务器 🚀