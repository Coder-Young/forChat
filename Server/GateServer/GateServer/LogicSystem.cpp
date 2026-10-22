#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyZmqClient.h"
#include "StatusZmqClient.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"

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
		std::cout << "[get_varifycode]receive body is " << body_str << std::endl;

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
		std::cout << "[get_varifycode]receive body is " << body_str << std::endl;

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

		auto user = src_root["user"].asString();
		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		int sex = src_root["sex"].asInt();
		auto icon = src_root["icon"].asString();
		auto nick = src_root["nick"].asString();
		auto cpwd = src_root["confirm"].asString();
		auto code = src_root["varifycode"].asString();

		if (pwd != cpwd)
		{
			std::cout << "pwd error" << std::endl;
        	root["error"] = ErrorCodes::PWDERROR;
			std::string jsonstr = root.toStyledString();
        	beast::ostream(connection->_response.body()) << jsonstr;
        	return true;
		}

		std::string stored_code;
		std::string key = std::string("verify_code") + ":" + email;
		RedisMgr::GetInstance()->Get(key, stored_code);
		//RedisMgr::GetInstance()->Del(key);
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
		
		int ret = MySqlMgr::GetInstance()->RegUser(user, email, pwd, sex, icon, nick);
		if (ret == 0 || ret == -1)
		{
			std::cout << "register user error" << std::endl;
        	root["error"] = ErrorCodes::REGERROR;
        	std::string jsonstr = root.toStyledString();
        	beast::ostream(connection->_response.body()) << jsonstr;
        	return true;
		}

		root["error"] = ErrorCodes::Success;
		root["uid"] = ret;
    	root["email"] = email;
    	root["user"] = user;
    	root["passwd"] = pwd;
    	root["confirm"] = cpwd;
    	root["varifycode"] = stored_code;
		root["sex"] = sex;
		root["icon"] = icon;
		root["nick"] = nick;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	RegisterPostFunc("/reset_pwd", [](std::shared_ptr<HttpConnection> connection){
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "[get_varifycode]receive body is " << body_str << std::endl;

		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success){
			std::cout << "[reset_pwd] failed to parse JSON" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return false;
		}

		auto user = src_root["user"].asString();
		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		auto vcode = src_root["varifycode"].asString();

		std::string ret_code = VerifyZmqClient::GetInstance()->RequestVerifyCode(email);
		if (ret_code.empty())
			return false;

		if (vcode != ret_code){
			std::cout << "[reset_pwd] varify failed" << std::endl;
			root["error"] = ErrorCodes::MISS_MATCH;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return false;
		}

		bool email_valid = MySqlMgr::GetInstance()->checkEmail(user, email);
		if (!email_valid){
			std::cout << "[reset_pwd]email not match" << std::endl;
			root["error"] = ErrorCodes::EMAIL_MISS_MATCH;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return false;
		}

		bool reset_success = MySqlMgr::GetInstance()->updatePwd(user, pwd);
		if (!reset_success){
			std::cout << "update pwd failed" << std::endl;
			root["error"] = ErrorCodes::UPDATE_PWD_FAIL;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return false;
		}

		std::cout << "succeed to update pwd" << std::endl;
		root["error"] = 0;
    	root["email"] = email;
    	root["user"] = user;
    	root["passwd"] = pwd;
    	root["varifycode"] = src_root["varifycode"].asString();
    	std::string jsonstr = root.toStyledString();
    	beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	RegisterPostFunc("/user_login", [](std::shared_ptr<HttpConnection> connection){
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "[user_login]failed to parse JSON" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return false;
		}

		auto user = src_root["user"].asString();
		auto pwd = src_root["pwd"].asString();
		UserInfo info;
		bool pwd_valid = MySqlMgr::GetInstance()->checkPwd(user, pwd, info);
		if (!pwd_valid)
		{
			std::cout << "[user_login]user pwd not match" << std::endl;
        	root["error"] = ErrorCodes::PWD_INVALID;
        	std::string jsonstr = root.toStyledString();
        	beast::ostream(connection->_response.body()) << jsonstr;
        	return false;
		}

		// 查询Status服务器找到合适连接
		Json::Value resp;
		bool ret = StatusZmqClient::GetInstance()->getChatServer(info.uid, resp);
		if (!ret)
		{
			std::cout << "[user_login]getChatServer failed" << std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return false;
		}

		std::cout << "success to load userinfo uid is" << info.uid << std::endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = info.email;
		root["uid"] = info.uid;
		root["token"] = resp["Token"];
		root["host"] = resp["Host"];
		root["port"] = resp["Port"];
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