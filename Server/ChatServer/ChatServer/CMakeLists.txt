cmake_minimum_required(VERSION 3.15)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

set(SOURCES
    main.cpp
    CServer.cpp
    CSession.cpp
    ConfigManager.cpp
    LogicSystem.cpp
    MySqlDao.cpp
    MySqlMgr.cpp
    MySqlPool.cpp
    RedisConPool.cpp
    RedisMgr.cpp
    StatusZmqClient.cpp
    ZmqConnectionPool.cpp
)

set(HEADERS
    const.h
    MsgNode.h
    Singleton.h
    CServer.h
    CSession.h
    ConfigManager.h
    LogicSystem.h
    MySqlDao.h
    MySqlMgr.h
    MySqlPool.h
    RedisConPool.h
    RedisMgr.h
    StatusZmqClient.h
    ZmqConnectionPool.h
    ZmqPoolManager.h
    ZmqConnectionGuard.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# json boost redis mysql zmq
find_package(jsoncpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)

find_package(ZMQ QUIET)
if(NOT ZMQ_FOUND)
    pkg_check_modules(ZMQ REQUIRED libzmq)
endif()

find_path(MYSQLCPPCONN_INCLUDE_DIRS cppconn/driver.h
    PATHS /usr/include /usr/local/include REQUIRED)

find_library(MYSQLCPPCONN_LIBRARIES mysqlcppconn
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib REQUIRED)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::JSONCPP
    ${Boost_LIBRARIES}
    PkgConfig::HIREDIS
    ${ZMQ_LIBRARIES}
    ${MYSQLCPPCONN_LIBRARIES}
)