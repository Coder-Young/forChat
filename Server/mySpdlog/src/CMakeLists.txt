set(MYSPDLOG_SOURCES
    level.cpp
    formatter.cpp
    pattern_formatter.cpp
    logger.cpp
    async_logger.cpp
    registry.cpp
    details/utils.cpp
    details/thread_pool.cpp
    sinks/rotating_file_sink.cpp
)

add_library(myspdlog STATIC ${MYSPDLOG_SOURCES})

target_include_directories(myspdlog PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(myspdlog PUBLIC fmt::fmt)

target_compile_features(myspdlog PUBLIC cxx_std_17)