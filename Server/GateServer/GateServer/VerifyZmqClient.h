#pragma once
#include "const.h"
#include "Singleton.h"

class VerifyZmqClient : public Singleton<VerifyZmqClient>
{
	friend Singleton<VerifyZmqClient>;
public:
	VerifyZmqClient();
	~VerifyZmqClient();

	std::string RequestVerifyCode(const std::string& email);

private:
	bool SendAndRecvMessage(const Json::Value& request, Json::Value& response);
};