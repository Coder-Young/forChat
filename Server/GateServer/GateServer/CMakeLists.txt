cmake_minimum_required(VERSION 3.15)
project(GateServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

set(SOURCES
    LogicSystem.cpp
    HttpConnection.cpp
    CServer.cpp
    GateServer.cpp
    AsioIOServicePool.cpp
    CfgManager.cpp
    ZmqConnectionPool.cpp
    VerifyZmqClient.cpp
    StatusZmqClient.cpp
    RedisMgr.cpp
    RedisConPool.cpp
    MySqlPool.cpp
    MySqlDao.cpp
    MySqlMgr.cpp
)

set(HEADERS
    Singleton.h
    const.h
    LogicSystem.h
    HttpConnection.h
    CServer.h
    AsioIOServicePool.h
    CfgManager.h
    ZmqPoolManager.h
    ZmqConnectionPool.h
    VerifyZmqClient.h
    StatusZmqClient.h
    RedisMgr.h
    RedisConPool.h
    MySqlPool.h
    MySqlDao.h
    MySqlMgr.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})


find_package(Boost REQUIRED COMPONENTS system filesystem) 

find_package(PkgConfig REQUIRED)
pkg_check_modules(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)

find_package(ZMQ QUIET)
if(NOT ZMQ_FOUND)
    pkg_check_modules(ZMQ REQUIRED libzmq)
endif()


find_path(MYSQLCPPCONN_INCLUDE_DIR cppconn/driver.h
    PATHS /usr/include /usr/local/include
    REQUIRED
)

find_library(MYSQLCPPCONN_LIBRARY mysqlcppconn
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
    REQUIRED
)

set(MYSQLCPPCONN_LIBRARIES ${MYSQLCPPCONN_LIBRARY})
set(MYSQLCPPCONN_INCLUDE_DIRS ${MYSQLCPPCONN_INCLUDE_DIR})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${MYSQLCPPCONN_LIBRARIES}
    ${ZMQ_LIBRARIES}
    ${Boost_LIBRARIES}
    PkgConfig::HIREDIS
    PkgConfig::JSONCPP
)