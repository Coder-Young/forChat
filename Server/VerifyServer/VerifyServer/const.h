#pragma once
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

struct EmailResult {
    bool success;
    std::string message;
};