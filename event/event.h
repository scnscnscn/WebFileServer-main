#pragma once

#include "../message/message.h"
#include "../utils/utils.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace webserver {

/**
 * @brief 事件处理基类
 * 
 * 采用现代C++设计，提供统一的事件处理接口
 * 使用智能指针管理资源，确保异常安全
 */
class EventHandler {
public:
    EventHandler() = default;
    virtual ~EventHandler() = default;
    
    // 禁用拷贝，启用移动
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = default;
    EventHandler& operator=(EventHandler&&) = default;
    
    /**
     * @brief 处理事件的纯虚函数
     * @throws std::exception 处理过程中的各种异常
     */
    virtual void process() = 0;

protected:
    // 使用智能指针管理连接状态，确保异常安全
    static std::unordered_map<int, std::unique_ptr<HttpRequest>> requestMap_;
    static std::unordered_map<int, std::unique_ptr<HttpResponse>> responseMap_;
    
    /**
     * @brief 获取或创建请求对象
     * @param fd 文件描述符
     * @return 请求对象的引用
     */
    static HttpRequest& getOrCreateRequest(int fd);
    
    /**
     * @brief 获取或创建响应对象
     * @param fd 文件描述符
     * @return 响应对象的引用
     */
    static HttpResponse& getOrCreateResponse(int fd);
    
    /**
     * @brief 清理连接相关资源
     * @param fd 文件描述符
     */
    static void cleanupConnection(int fd);
};

/**
 * @brief 接受新连接事件处理器
 */
class AcceptHandler : public EventHandler {
public:
    AcceptHandler(int listenFd, int epollFd) 
        : listenFd_(listenFd), epollFd_(epollFd) {}
    
    void process() override;

private:
    int listenFd_;
    int epollFd_;
};

/**
 * @brief 接收数据事件处理器
 */
class ReceiveHandler : public EventHandler {
public:
    ReceiveHandler(int clientFd, int epollFd) 
        : clientFd_(clientFd), epollFd_(epollFd) {}
    
    void process() override;

private:
    int clientFd_;
    int epollFd_;
    
    /**
     * @brief 处理HTTP请求行
     * @param request 请求对象
     * @return 是否处理完成
     */
    bool processRequestLine(HttpRequest& request);
    
    /**
     * @brief 处理HTTP头部
     * @param request 请求对象
     * @return 是否处理完成
     */
    bool processHeaders(HttpRequest& request);
    
    /**
     * @brief 处理HTTP消息体
     * @param request 请求对象
     * @return 是否处理完成
     */
    bool processBody(HttpRequest& request);
    
    /**
     * @brief 处理文件上传
     * @param request 请求对象
     * @return 是否处理完成
     */
    bool processFileUpload(HttpRequest& request);
    
    /**
     * @brief URL解码
     * @param encoded 编码的字符串
     * @return 解码后的字符串
     */
    std::string urlDecode(const std::string& encoded);
};

/**
 * @brief 发送数据事件处理器
 */
class SendHandler : public EventHandler {
public:
    SendHandler(int clientFd, int epollFd) 
        : clientFd_(clientFd), epollFd_(epollFd) {}
    
    void process() override;

private:
    int clientFd_;
    int epollFd_;
    
    /**
     * @brief 构建响应内容
     * @param response 响应对象
     * @param uri 请求URI
     */
    void buildResponse(HttpResponse& response, const std::string& uri);
    
    /**
     * @brief 生成文件列表HTML页面
     * @return HTML内容
     */
    std::string generateFileListHtml();
    
    /**
     * @brief 获取指定目录下的文件列表
     * @param dirPath 目录路径
     * @return 文件名列表
     */
    std::vector<std::string> getFileList(const std::string& dirPath);
    
    /**
     * @brief 发送HTTP头部
     * @param response 响应对象
     * @return 是否发送完成
     */
    bool sendHeaders(HttpResponse& response);
    
    /**
     * @brief 发送消息体
     * @param response 响应对象
     * @return 是否发送完成
     */
    bool sendBody(HttpResponse& response);
    
    /**
     * @brief 发送HTML内容
     * @param response 响应对象
     * @return 是否发送完成
     */
    bool sendHtmlContent(HttpResponse& response);
    
    /**
     * @brief 发送文件内容
     * @param response 响应对象
     * @return 是否发送完成
     */
    bool sendFileContent(HttpResponse& response);
};

} // namespace webserver