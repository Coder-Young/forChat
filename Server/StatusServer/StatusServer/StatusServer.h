#pragma once
#include "const.h"
#include <zmq.hpp>

typedef std::function<void(Json::Value&)> HandleFunc;

struct ChatServer{
    ChatServer() :
        host(""),
        port(""),
        con_count(0),
        name("")
    {}

    std::string host;
    std::string port;
    int con_count;
    std::string name;
};

class StatusServer
{
public:
    StatusServer(std::string &bind_ep);
    ~StatusServer();

    void Start();

    void HandleGetChatServer(Json::Value&);
    void HandleVerifyToken(Json::Value&);

private:
    ChatServer getChatServer();

    void insertIntoRedis(int uid, std::string token);

    void SendReply(const Json::Value& reply);

private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::string bind_endpoint_;

    std::unordered_map<std::string, ChatServer> _servers;
    std::unordered_map<int, std::string> _tokens;
    std::unordered_map<std::string, HandleFunc> _handler;
    std::vector<zmq::message_t> current_identity_;
    zmq::message_t identity_;
    std::mutex _mutex;

    std::atomic<bool> b_stop;
    int server_index;
    std::thread worker_thread_;
};