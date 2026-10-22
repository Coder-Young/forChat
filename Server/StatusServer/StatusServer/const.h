#pragma once
#include <memory>
#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <zmq.hpp>
#include <condition_variable>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <hiredis/hiredis.h>
#include <myspdlog/myspdlog.h>

#define USERTOKENPREFIX "USERTOKEN_"
#define LOGIN_COUNT "LOGINCOUNT_"
#define IPCOUNTPERFIX "IPCOUNT_"
#define USERIPPERFIX "USERIP_"
#define USERBASEINFO "USERBASEINFO_"
#define LOCK_PREFIX "lock_"

#define LOCK_TIME_OUT 10
#define ACQUIRE_TIME_OUT 5

enum ErrorCodes{
    Success      =      1001,
    InvalidParam =      1002,
    InternalError =     1003,
    UidInvalid  =       1004,
    TokenInvalid  =     1005,
};

class Defer
{
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer()
    {
        func_();
    }

private:
    std::function<void()> func_;
};