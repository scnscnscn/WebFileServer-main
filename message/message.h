#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

namespace webserver {

/**
 * @brief HTTP消息处理状态枚举
 */
enum class MessageStatus {
    INIT,       ///< 初始状态，等待处理请求行
    HEADERS,    ///< 处理HTTP头部
    BODY,       ///< 处理消息体
    COMPLETE,   ///< 处理完成
    ERROR       ///< 处理出错
};

/**
 * @brief HTTP响应消息体类型
 */
enum class BodyType {
    FILE,       ///< 文件类型
    HTML,       ///< HTML页面
    EMPTY       ///< 空消息体
};

/**
 * @brief 文件上传处理状态
 */
enum class FileUploadStatus {
    BOUNDARY,   ///< 查找边界标识
    HEADERS,    ///< 处理文件头部信息
    CONTENT,    ///< 处理文件内容
    COMPLETE    ///< 文件处理完成
};

/**
 * @brief HTTP消息基类
 * 
 * 提供HTTP请求和响应消息的公共接口和数据结构
 */
class HttpMessage {
public:
    HttpMessage() = default;
    virtual ~HttpMessage() = default;
    
    // 禁用拷贝，启用移动
    HttpMessage(const HttpMessage&) = delete;
    HttpMessage& operator=(const HttpMessage&) = delete;
    HttpMessage(HttpMessage&&) = default;
    HttpMessage& operator=(HttpMessage&&) = default;
    
    MessageStatus getStatus() const noexcept { return status_; }
    void setStatus(MessageStatus status) noexcept { status_ = status; }
    
    const std::unordered_map<std::string, std::string>& getHeaders() const noexcept {
        return headers_;
    }
    
    std::optional<std::string> getHeader(const std::string& key) const {
        auto it = headers_.find(key);
        return (it != headers_.end()) ? std::make_optional(it->second) : std::nullopt;
    }
    
    void setHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }

protected:
    MessageStatus status_{MessageStatus::INIT};
    std::unordered_map<std::string, std::string> headers_;
};

/**
 * @brief HTTP请求消息类
 * 
 * 封装HTTP请求的解析和状态管理
 */
class HttpRequest : public HttpMessage {
public:
    HttpRequest() = default;
    
    /**
     * @brief 解析HTTP请求行
     * @param requestLine 请求行字符串 (如: "GET /index.html HTTP/1.1")
     * @throws std::invalid_argument 请求行格式错误时抛出
     */
    void parseRequestLine(const std::string& requestLine);
    
    /**
     * @brief 解析HTTP头部字段
     * @param headerLine 头部行字符串 (如: "Content-Type: text/html")
     * @throws std::invalid_argument 头部格式错误时抛出
     */
    void parseHeaderLine(const std::string& headerLine);
    
    // Getters
    const std::string& getMethod() const noexcept { return method_; }
    const std::string& getUri() const noexcept { return uri_; }
    const std::string& getVersion() const noexcept { return version_; }
    size_t getContentLength() const noexcept { return contentLength_; }
    
    // 文件上传相关
    const std::string& getFileName() const noexcept { return fileName_; }
    void setFileName(const std::string& fileName) { fileName_ = fileName; }
    
    FileUploadStatus getFileUploadStatus() const noexcept { return fileUploadStatus_; }
    void setFileUploadStatus(FileUploadStatus status) noexcept { fileUploadStatus_ = status; }
    
    // 接收缓冲区管理
    std::string& getReceiveBuffer() noexcept { return receiveBuffer_; }
    const std::string& getReceiveBuffer() const noexcept { return receiveBuffer_; }
    void clearReceiveBuffer() { receiveBuffer_.clear(); }

private:
    std::string method_;        ///< HTTP方法 (GET, POST等)
    std::string uri_;           ///< 请求URI
    std::string version_;       ///< HTTP版本
    size_t contentLength_{0};   ///< 消息体长度
    
    // 文件上传相关
    std::string fileName_;
    FileUploadStatus fileUploadStatus_{FileUploadStatus::BOUNDARY};
    
    // 接收缓冲区
    std::string receiveBuffer_;
};

/**
 * @brief HTTP响应消息类
 * 
 * 封装HTTP响应的构建和发送状态管理
 */
class HttpResponse : public HttpMessage {
public:
    HttpResponse() = default;
    
    /**
     * @brief 设置HTTP状态行
     * @param version HTTP版本
     * @param statusCode 状态码
     * @param reasonPhrase 状态描述
     */
    void setStatusLine(const std::string& version, int statusCode, 
                      const std::string& reasonPhrase);
    
    /**
     * @brief 构建完整的HTTP响应头部
     * @return 响应头部字符串
     */
    std::string buildHeaders() const;
    
    // Getters and Setters
    const std::string& getVersion() const noexcept { return version_; }
    int getStatusCode() const noexcept { return statusCode_; }
    const std::string& getReasonPhrase() const noexcept { return reasonPhrase_; }
    
    BodyType getBodyType() const noexcept { return bodyType_; }
    void setBodyType(BodyType type) noexcept { bodyType_ = type; }
    
    const std::string& getBodyContent() const noexcept { return bodyContent_; }
    void setBodyContent(const std::string& content) { bodyContent_ = content; }
    
    const std::string& getFilePath() const noexcept { return filePath_; }
    void setFilePath(const std::string& path) { filePath_ = path; }
    
    int getFileFd() const noexcept { return fileFd_; }
    void setFileFd(int fd) noexcept { fileFd_ = fd; }
    
    size_t getContentLength() const noexcept { return contentLength_; }
    void setContentLength(size_t length) noexcept { contentLength_ = length; }
    
    size_t getSentBytes() const noexcept { return sentBytes_; }
    void addSentBytes(size_t bytes) noexcept { sentBytes_ += bytes; }
    void resetSentBytes() noexcept { sentBytes_ = 0; }

private:
    std::string version_{"HTTP/1.1"};
    int statusCode_{200};
    std::string reasonPhrase_{"OK"};
    
    BodyType bodyType_{BodyType::EMPTY};
    std::string bodyContent_;       ///< HTML内容
    std::string filePath_;          ///< 文件路径
    int fileFd_{-1};               ///< 文件描述符
    
    size_t contentLength_{0};       ///< 内容长度
    size_t sentBytes_{0};          ///< 已发送字节数
};

} // namespace webserver