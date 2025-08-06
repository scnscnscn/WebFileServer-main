#include "message.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace webserver {

void HttpRequest::parseRequestLine(const std::string& requestLine) {
    std::istringstream iss(requestLine);
    if (!(iss >> method_ >> uri_ >> version_)) {
        throw std::invalid_argument("Invalid request line format");
    }
    
    // 验证HTTP版本格式
    if (version_.substr(0, 5) != "HTTP/") {
        throw std::invalid_argument("Invalid HTTP version");
    }
}

void HttpRequest::parseHeaderLine(const std::string& headerLine) {
    auto colonPos = headerLine.find(':');
    if (colonPos == std::string::npos) {
        throw std::invalid_argument("Invalid header line format");
    }
    
    std::string key = headerLine.substr(0, colonPos);
    std::string value = headerLine.substr(colonPos + 1);
    
    // 去除前后空白字符
    key.erase(key.find_last_not_of(" \t\r\n") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);
    
    // 特殊处理某些头部字段
    if (key == "Content-Length") {
        try {
            contentLength_ = std::stoull(value);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid Content-Length value");
        }
    } else if (key == "Content-Type") {
        // 处理multipart/form-data; boundary=xxx格式
        auto semicolonPos = value.find(';');
        if (semicolonPos != std::string::npos) {
            std::string contentType = value.substr(0, semicolonPos);
            std::string params = value.substr(semicolonPos + 1);
            
            headers_[key] = contentType;
            
            // 解析boundary参数
            auto boundaryPos = params.find("boundary=");
            if (boundaryPos != std::string::npos) {
                std::string boundary = params.substr(boundaryPos + 9);
                boundary.erase(0, boundary.find_first_not_of(" \t"));
                headers_["boundary"] = boundary;
            }
        } else {
            headers_[key] = value;
        }
    } else {
        headers_[key] = value;
    }
}

void HttpResponse::setStatusLine(const std::string& version, int statusCode, 
                                const std::string& reasonPhrase) {
    version_ = version;
    statusCode_ = statusCode;
    reasonPhrase_ = reasonPhrase;
}

std::string HttpResponse::buildHeaders() const {
    std::ostringstream oss;
    
    // 状态行
    oss << version_ << " " << statusCode_ << " " << reasonPhrase_ << "\r\n";
    
    // 头部字段
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    
    // 空行
    oss << "\r\n";
    
    return oss.str();
}

} // namespace webserver