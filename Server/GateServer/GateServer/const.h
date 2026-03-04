#pragma once
#include <memory>
#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <unordered_map>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

#include "Singleton.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

enum ErrorCodes
{
	Success		= 0,
	Error_Json	= 1001,
	RFCFailed	= 1002,
};