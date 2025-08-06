#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>

namespace webserver {

/**
 * @brief HTTP方法枚举
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    UNKNOWN
};

/**
 * @brief HTTP版本枚举
 */
enum class HttpVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0,
    UNKNOWN
};

/**
 * @brief HTTP解析状态
 */
enum class ParseState {
    REQUEST_LINE,    ///< 解析请求行
    HEADERS,         ///< 解析头部
    BODY,           ///< 解析消息体
    COMPLETE,       ///< 解析完成
    ERROR           ///< 解析错误
};

/**
 * @brief HTTP请求类
 */
class HttpRequest {
public:
    HttpRequest() = default;
    ~HttpRequest() = default;
    
    // Getters
    HttpMethod getMethod() const noexcept { return method_; }
    const std::string& getUri() const noexcept { return uri_; }
    const std::string& getPath() const noexcept { return path_; }
    const std::string& getQuery() const noexcept { return query_; }
    HttpVersion getVersion() const noexcept { return version_; }
    
    const std::unordered_map<std::string, std::string>& getHeaders() const noexcept {
        return headers_;
    }
    
    std::optional<std::string> getHeader(const std::string& name) const {
        auto it = headers_.find(name);
        return (it != headers_.end()) ? std::make_optional(it->second) : std::nullopt;
    }
    
    const std::string& getBody() const noexcept { return body_; }
    size_t getContentLength() const noexcept { return contentLength_; }
    
    bool isKeepAlive() const noexcept { return keepAlive_; }
    
    // Setters (for parser use)
    void setMethod(HttpMethod method) noexcept { method_ = method; }
    void setUri(const std::string& uri) { uri_ = uri; parseUri(); }
    void setVersion(HttpVersion version) noexcept { version_ = version; }
    void addHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body) { body_ = body; }
    
    /**
     * @brief 重置请求对象以供重用
     */
    void reset();
    
    /**
     * @brief 获取请求的字符串表示
     * @return 请求字符串
     */
    std::string toString() const;

private:
    /**
     * @brief 解析URI，提取路径和查询参数
     */
    void parseUri();
    
    /**
     * @brief 更新Keep-Alive状态
     */
    void updateKeepAlive();

private:
    HttpMethod method_{HttpMethod::UNKNOWN};
    std::string uri_;
    std::string path_;
    std::string query_;
    HttpVersion version_{HttpVersion::UNKNOWN};
    
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    
    size_t contentLength_{0};
    bool keepAlive_{false};
};

/**
 * @brief HTTP响应类
 */
class HttpResponse {
public:
    HttpResponse() = default;
    ~HttpResponse() = default;
    
    // Getters
    HttpVersion getVersion() const noexcept { return version_; }
    int getStatusCode() const noexcept { return statusCode_; }
    const std::string& getReasonPhrase() const noexcept { return reasonPhrase_; }
    
    const std::unordered_map<std::string, std::string>& getHeaders() const noexcept {
        return headers_;
    }
    
    const std::string& getBody() const noexcept { return body_; }
    
    // Setters
    void setVersion(HttpVersion version) noexcept { version_ = version; }
    void setStatus(int code, const std::string& phrase) {
        statusCode_ = code;
        reasonPhrase_ = phrase;
    }
    void addHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
    }
    void setBody(const std::string& body) { body_ = body; }
    
    /**
     * @brief 构建完整的HTTP响应字符串
     * @return 响应字符串
     */
    std::string toString() const;
    
    /**
     * @brief 重置响应对象以供重用
     */
    void reset();

private:
    HttpVersion version_{HttpVersion::HTTP_1_1};
    int statusCode_{200};
    std::string reasonPhrase_{"OK"};
    
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

/**
 * @brief HTTP解析器
 * 
 * 负责解析HTTP请求，支持分块解析和状态保持
 */
class HttpParser {
public:
    HttpParser() = default;
    ~HttpParser() = default;
    
    /**
     * @brief 解析HTTP请求数据
     * @param data 数据缓冲区
     * @param length 数据长度
     * @param request 请求对象
     * @return 解析状态
     */
    ParseState parse(const char* data, size_t length, HttpRequest& request);
    
    /**
     * @brief 获取当前解析状态
     * @return 解析状态
     */
    ParseState getState() const noexcept { return state_; }
    
    /**
     * @brief 重置解析器状态
     */
    void reset();
    
    /**
     * @brief 获取错误信息
     * @return 错误信息
     */
    const std::string& getError() const noexcept { return error_; }

private:
    /**
     * @brief 解析请求行
     * @param line 请求行
     * @param request 请求对象
     * @return 是否解析成功
     */
    bool parseRequestLine(const std::string& line, HttpRequest& request);
    
    /**
     * @brief 解析头部行
     * @param line 头部行
     * @param request 请求对象
     * @return 是否解析成功
     */
    bool parseHeaderLine(const std::string& line, HttpRequest& request);
    
    /**
     * @brief 字符串转HTTP方法
     * @param method 方法字符串
     * @return HTTP方法枚举
     */
    static HttpMethod stringToMethod(const std::string& method);
    
    /**
     * @brief 字符串转HTTP版本
     * @param version 版本字符串
     * @return HTTP版本枚举
     */
    static HttpVersion stringToVersion(const std::string& version);
    
    /**
     * @brief HTTP方法转字符串
     * @param method HTTP方法
     * @return 方法字符串
     */
    static std::string methodToString(HttpMethod method);
    
    /**
     * @brief HTTP版本转字符串
     * @param version HTTP版本
     * @return 版本字符串
     */
    static std::string versionToString(HttpVersion version);

private:
    ParseState state_{ParseState::REQUEST_LINE};
    std::string buffer_;                    ///< 解析缓冲区
    std::string error_;                     ///< 错误信息
    size_t expectedBodyLength_{0};          ///< 期望的消息体长度
    size_t receivedBodyLength_{0};          ///< 已接收的消息体长度
};

} // namespace webserver