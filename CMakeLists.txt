cmake_minimum_required(VERSION 3.12)
project(WebFileServer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# 查找依赖库
find_package(Threads REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})

# 源文件
set(SOURCES
    main.cpp
    src/core/server.cpp
    src/config/server_config.cpp
    src/threadpool/thread_pool.cpp
    src/utils/logger.cpp
    src/utils/socket_utils.cpp
    src/network/connection.cpp
    src/network/connection_manager.cpp
    src/http/http_parser.cpp
    src/event/event_handlers.cpp
    src/file/file_handler.cpp
)

# 头文件
set(HEADERS
    src/core/server.h
    src/config/server_config.h
    src/threadpool/thread_pool.h
    src/utils/logger.h
    src/utils/socket_utils.h
    src/network/connection.h
    src/network/connection_manager.h
    src/http/http_parser.h
    src/event/event_handlers.h
    src/file/file_handler.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    Threads::Threads
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 安装资源文件
install(DIRECTORY html/ DESTINATION share/${PROJECT_NAME}/html)
install(DIRECTORY filedir/ DESTINATION share/${PROJECT_NAME}/filedir)

# 创建配置文件示例
configure_file(
    ${CMAKE_SOURCE_DIR}/config/server.conf.example
    ${CMAKE_BINARY_DIR}/server.conf.example
    COPYONLY
)

install(FILES ${CMAKE_BINARY_DIR}/server.conf.example
    DESTINATION etc/${PROJECT_NAME}
)

# 测试
enable_testing()

# 添加单元测试（如果有的话）
if(EXISTS ${CMAKE_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()

# 打包配置
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++ Web File Server")
set(CPACK_PACKAGE_VENDOR "WebServer Team")
set(CPACK_PACKAGE_CONTACT "admin@webserver.com")

include(CPack)