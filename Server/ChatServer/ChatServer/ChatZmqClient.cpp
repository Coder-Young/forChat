#include "ChatZmqClient.h"
#include "ConfigManager.h"
#include "ZmqPoolManager.h"
#include "ZmqConnectionGuard.h"

ChatZmqClient::ChatZmqClient()
{
    CfgManager cfg;
    auto server_list = cfg["PeerSever"]["Servers"];

    std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

    while (std::getline(ss, word, ',')){
        words.push_back(word);
    }

    for (auto &server : words){
        std::string server_name = cfg[server.c_str()]["Name"];
        if (server_name.empty()){
            continue;
        }
        
        _pool[server_name] = ZmqPoolManager::GetInstance()->getPool(server_name);
    }
}

bool ChatZmqClient::NotifyAddFriend(const std::string& server_ip, const Json::Value& req)
{
    if (_pool.find(server_ip) == _pool.end()){
        return false;
    }

    Json::Value request = req;
    request["MsgID"] = MSG_ID::ID_ADD_FRIEND_REQ;
    std::string jsonstr = request.toStyledString();
    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    ZmqConnectionGuard guard(*_pool[server_ip]);
    auto dealer = guard.get();
    if (dealer == nullptr){
        return false;
    }

    try{
        auto send_res = dealer->send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()){
            std::cerr << "[ChatZmqClient] Failed to send request." << std::endl;
            return false;
        }

        return true;
    }
    catch(const std::exception& e){
        std::cout << "[ChatZmqClient] error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

bool ChatZmqClient::NotifyAcceptFriend(const std::string& server_ip, const Json::Value& req)
{
    if (_pool.find(server_ip) == _pool.end()){
        return false;
    }

    Json::Value request = req;
    request["MsgID"] = MSG_ID::ID_AUTH_FRIEND_REQ;
    std::string jsonstr = request.toStyledString();
    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    ZmqConnectionGuard guard(*_pool[server_ip]);
    auto dealer = guard.get();
    if (dealer == nullptr){
        return false;
    }

    try{
        auto send_res = dealer->send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()){
            std::cerr << "[ChatZmqClient] Failed to send request." << std::endl;
            return false;
        }

        return true;
    }
    catch(const std::exception& e){
        std::cout << "[ChatZmqClient] error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

bool ChatZmqClient::NotifyKickUser(const std::string&, const Json::Value&)
{
    
}