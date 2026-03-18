cmake_minimum_required(VERSION 3.10)
project(VerifyServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

include(FindPkgConfig)
pkg_check_modules(CURL REQUIRED libcurl)

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
    main.cpp
    EmailService.cpp
    VerifyServer.cpp
    RedisMgr.cpp
)

set(HEADERS
    Singleton.h
    const.h
    VerifyServer.h
    EmailService.h
    RedisMgr.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE hiredis)

if(JSONCPP_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${JSONCPP_INCLUDE_DIRS})
endif()

if (TARGET ZMQ::ZMQ)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ZMQ::ZMQ
    )
else(ZMQ_FOUND_BY_PKGCONFIG OR ZMQ_LIBRARIES)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARIES})
endif()

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

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::HIREDIS)

target_link_libraries(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})
target_link_directories(${PROJECT_NAME} PRIVATE ${CURL_LIBRARY_DIRS})

# 编译选项 (可选，增加警告信息)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)