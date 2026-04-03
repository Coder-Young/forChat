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

#define USERTOKENPREFIX "USERTOKEN_"

enum ErrorCodes{
    Success      =      1001,
    InvalidParam =      1002,
    InternalError =     1003,
    UidInvalid  =       1004,
    TokenInvalid  =     1005,
};