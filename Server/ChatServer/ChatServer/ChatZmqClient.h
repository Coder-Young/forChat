#pragma once
#include "const.h"
#include "ZmqConnectionPool.h"
#include "Singleton.h"

class ChatZmqClient : public Singleton<ChatZmqClient>
{
    friend class Singleton<ChatZmqClient>;
public:
    ChatZmqClient();

    bool NotifyAddFriend(const std::string&, const Json::Value&);
    bool NotifyAcceptFriend(const std::string&, const Json::Value&);
    bool NotifyKickUser(const std::string&, const Json::Value&);

private:
    std::unordered_map<std::string, std::shared_ptr<ZmqConnectionPool>> _pool;
};