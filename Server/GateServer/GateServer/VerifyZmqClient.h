#pragma once
#include "const.h"
#include <zmq.hpp>

class VerifyZmqClient : public Singleton<VerifyZmqClient>
{
	friend Singleton<VerifyZmqClient>;
public:
	VerifyZmqClient();

	std::string RequestVerifyCode(const std::string& email);

private:
	bool SendAndRecvMessage(const Json::Value& request, Json::Value& response);

private:
	zmq::context_t context_;
	zmq::socket_t dealer_;
	std::string endpoint_;
};