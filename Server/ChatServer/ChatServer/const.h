#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <functional>
#include <zmq.hpp>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <hiredis/hiredis.h>

#define USERTOKENPREFIX "USERTOKEN_"
#define MAX_MSG_SIZE    4096
#define MAX_SENDQUE_SIZE 1000
#define HEAD_ID_LEN     2
#define HEAD_DATA_LEN   2
#define HEAD_LEN        (HEAD_ID_LEN+HEAD_DATA_LEN)

enum MSG_ID{
    MSG_CHAT_LOGIN_RSP  =   1005,
    MSG_CHAT_LOGIN      =   1006,
};

enum ErrorCodes{
    Success =         1001,
    UidInvalid =      1002,
    TokenInvalid =    1003,
    VerifyFailed =    1004,
};

struct UserInfo{
	std::string name;
	std::string email;
	std::string uid;
	std::string pwd;
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