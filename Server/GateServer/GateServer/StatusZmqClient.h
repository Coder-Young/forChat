#pragma once
#include "const.h"
#include "Singleton.h"

class StatusZmqClient : public Singleton<StatusZmqClient>
{
    friend Singleton<StatusZmqClient>;
public:
    StatusZmqClient();
    ~StatusZmqClient();

    bool getChatServer(const int& uid, Json::Value &response);
};