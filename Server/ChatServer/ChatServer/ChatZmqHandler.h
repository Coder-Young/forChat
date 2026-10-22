#pragma once
#include "const.h"

class CServer;
class ChatZmqHandler
{
public:
    ChatZmqHandler(const std::string& endpoint);

    void registerHandler();

    void StartHandling();
    void HandleAddFriend(const Json::Value& request);
    void HandleAcceptFriend(const Json::Value& request);
    void HandleKickUser(const Json::Value& request);

    void SendReply(const Json::Value& reply);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

    void RegisterServer(std::shared_ptr<CServer> pServer){
        _p_server = pServer;
    }

private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::string endpoint_; 

    zmq::message_t identity_;

    std::unordered_map<int, std::function<void(const Json::Value&)>> _handlers;

    std::shared_ptr<CServer> _p_server;
};