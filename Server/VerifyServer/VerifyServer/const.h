#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <string>
#include <random>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <curl/curl.h>
#include <zmq.hpp>

struct EmailResult {
    bool success;
    std::string message;
};