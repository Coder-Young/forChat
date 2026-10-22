#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <string>
#include <random>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <curl/curl.h>
#include <zmq.hpp>
#include <hiredis/hiredis.h>
#include <myspdlog/myspdlog.h>

struct EmailResult {
    bool success;
    std::string message;
};

#define EXPIRE_TIME     600