#pragma once
#include "const.h"
#include "Singleton.h"
#include "ZmqConnectionPool.h"

class ChatZmqClient : public Singleton<ChatZmqClient>
{
public:
    ChatZmqClient();
private:
    std::unordered_map<std::string, std::shared_ptr<ZmqConnectionPool>> _pool;
};