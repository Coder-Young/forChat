cmake_minimum_required(VERSION 3.15)
project(StatusServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

set(HEADERS
    const.h
    CfgManager.h
    RedisConPool.h
    RedisMgr.h
    Singleton.h
    StatusServer.h
    RedisConnectionGuard.h
)

set(SOURCES
    main.cpp
    CfgManager.cpp
    RedisConPool.cpp
    RedisMgr.cpp
    StatusServer.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# redis boost json zmq
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
pkg_check_modules(ZMQ REQUIRED libzmq)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${ZMQ_LIBRARIES}
    ${Boost_LIBRARIES}
    PkgConfig::HIREDIS
    PkgConfig::JSONCPP
)