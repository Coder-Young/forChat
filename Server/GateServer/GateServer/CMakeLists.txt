cmake_minimum_required(VERSION 3.15)
project(GateServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#if(NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
#endif()
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
find_package(Boost REQUIRED COMPONENTS system filesystem) 
find_package(jsoncpp CONFIG QUIET)
if(NOT jsoncpp_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

find_package(ZMQ CONFIG QUIET)
if(NOT ZMQ_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZMQ REQUIRED libzmq)
    set(ZMQ_FOUND_BY_PKGCONFIG TRUE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)

set(SOURCES
    LogicSystem.cpp
    HttpConnection.cpp
    CServer.cpp
    GateServer.cpp
    AsioIOServicePool.cpp
    CfgManager.cpp
    VerifyZmqClient.cpp
    RedisMgr.cpp
    RedisConPool.cpp
)

set(HEADERS
    Singleton.h
    const.h
    LogicSystem.h
    HttpConnection.h
    CServer.h
    AsioIOServicePool.h
    CfgManager.h
    VerifyZmqClient.h
    RedisMgr.h
    RedisConPool.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME} PRIVATE hiredis)

# 如果 jsoncpp 是通过 pkg_config 找到的，需要手动添加包含目录
if(JSONCPP_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${JSONCPP_INCLUDE_DIRS})
endif()

# 链接库
if (TARGET ZMQ::ZMQ)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ZMQ::ZMQ
    )
else(ZMQ_FOUND_BY_PKGCONFIG OR ZMQ_LIBRARIES)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system
    Boost::filesystem
)

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::HIREDIS)

# 处理 jsoncpp 的链接方式
if(TARGET jsoncpp_lib)
    # CMake 现代目标导入方式
    target_link_libraries(${PROJECT_NAME} PRIVATE jsoncpp_lib)
elseif(JSONCPP_LIBRARIES)
    # 传统变量方式
    target_link_libraries(${PROJECT_NAME} PRIVATE ${JSONCPP_LIBRARIES})
else()
    # 如果以上都失败，尝试直接链接名字 (可能需要在系统中安装 libjsoncpp-dev)
    target_link_libraries(${PROJECT_NAME} PRIVATE jsoncpp)
endif()

# 编译选项 (可选，增加警告信息)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

# 线程支持 (Boost.Asio 有时需要 pthread)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)