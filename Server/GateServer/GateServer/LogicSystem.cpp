#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyZmqClient.h"
#include "RedisMgr.h"

LogicSystem::LogicSystem()
{
	RegisterGetFunc("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		boost::beast::ostream(connection->_response.body()) << "receive get_test req";
		int i = 0;
		for (auto& elem : connection->_get_params)
		{
			i++;
			beast::ostream(connection->_response.body()) << "param " << i << "key is " << elem.first << " value is " << elem.second << std::endl;
		}
	});

	RegisterPostFunc("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;

		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;

		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "failed to parse getEmail Json" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email"))
		{
			std::cout << "failed to parse Json" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		std::string ret_code = VerifyZmqClient::GetInstance()->RequestVerifyCode(email);
		if (ret_code.empty())
			return false;
		
		std::cout << "email is " << email << std::endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	RegisterPostFunc("/user_register", [](std::shared_ptr<HttpConnection> connection){
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;

		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;

		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "failed to parse register Json" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("varifycode") || !src_root.isMember("email"))
		{
			std::cout << "failed to parse register Json" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto code = src_root["varifycode"].asString();
		std::string stored_code;
		std::string key = std::string("verify_code") + ":" + email;
		RedisMgr::GetInstance()->Get(key, stored_code);

		if (stored_code.empty())
		{
        	std::cout << "No verification code found" << std::endl;
        	root["error"] = ErrorCodes::EXPIRED;
			std::string jsonstr = root.toStyledString();
        	beast::ostream(connection->_response.body()) << jsonstr;
        	return true;
		}

		if (stored_code != code)
		{
			std::cout << "Verification code mismatch!" << std::endl;
        	root["error"] = ErrorCodes::MISS_MATCH;
			std::string jsonstr = root.toStyledString();
        	beast::ostream(connection->_response.body()) << jsonstr;
        	return true;
		}

		RedisMgr::GetInstance()->Del(key);
		root["error"] = ErrorCodes::Success;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con)
{
	if (_get_handlers.find(path) == _get_handlers.end())
		return false;

	_get_handlers[path](con);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con)
{
	if (_post_handlers.find(path) == _post_handlers.end())
		return false;

	_post_handlers[path](con);
	return true;
}