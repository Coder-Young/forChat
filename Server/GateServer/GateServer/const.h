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
#include <zmq.hpp>
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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

enum ErrorCodes
{
	Success				= 0,
	Error_Json			= 1001,
	RFCFailed			= 1002,
	MISS_MATCH			= 1003,
	EXPIRED 			= 1004,
	PWDERROR 			= 1005,
	REGERROR 			= 1006,
	EMAIL_MISS_MATCH 	= 1007,
	UPDATE_PWD_FAIL 	= 1008,
	PWD_INVALID			= 1009,
	RPCFailed			= 1010,
};

struct UserInfo {
	UserInfo() :
        name(""), pwd(""),uid(0),email(""),nick(""),desc(""),sex(0), icon(""), back("")
    {}

	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string nick;
	std::string desc;
	int sex;
	std::string icon;
	std::string back;
};