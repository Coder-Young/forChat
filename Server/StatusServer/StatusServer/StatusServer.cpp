#include "StatusServer.h"
#include "CfgManager.h"
#include "const.h"
#include "RedisMgr.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::string unique_string = to_string(uuid);
    return unique_string;
}

StatusServer::StatusServer(std::string &bind_ep)
    : context_(1), socket_(context_, ZMQ_ROUTER), bind_endpoint_(bind_ep), b_stop(false)
{
    CfgManager cfg;
    ChatServer s;
    s.port = cfg["ChatServer1"]["Port"];
    s.host = cfg["ChatServer1"]["Host"];
    s.con_count = 0;
    s.name = cfg["ChatServer1"]["Name"];
    _servers.insert(std::make_pair(s.name, s));

    // s.port = cfg["ChatServer2"]["Port"];
    // s.host = cfg["ChatServer2"]["Host"];
    // s.con_count = 0;
    // s.name = cfg["ChatServer2"]["Name"];
    // _servers.insert(std::make_pair(s.name, s));

    try
    {
        socket_.bind(bind_endpoint_);
    }
    catch(const zmq::error_t& e)
    {
        std::cerr << "ZMQ bind error: " << e.what() 
              << " (errno: " << e.num() << ")" << std::endl;
    }
    
    std::cout << "[StatusServer]Bind to " << bind_endpoint_ << std::endl;

    _handler.insert(std::make_pair("GetChatServer", 
        std::bind(&StatusServer::HandleGetChatServer, this, std::placeholders::_1)));

    _handler.insert(std::make_pair("VerifyToken", 
        std::bind(&StatusServer::HandleVerifyToken, this, std::placeholders::_1)));
}

StatusServer::~StatusServer()
{
    if (worker_thread_.joinable()){
        b_stop = true;
        worker_thread_.join();
    }
}

void StatusServer::Start()
{
    static int test_count = 0;
    b_stop = false;
    worker_thread_ = std::thread([this](){
    while (!b_stop)
    {
        try{
			auto id_res = socket_.recv(identity_, zmq::recv_flags::none);
			if (!id_res.has_value())
				continue;

            zmq::message_t data_msg;
			auto data_res = socket_.recv(data_msg, zmq::recv_flags::none);
			if (!data_res.has_value())
				continue;
            // std::vector<zmq::message_t> parts;
            // while (true){
            //     zmq::message_t part;
            //     if (!socket_.recv(part)){
            //         std::cerr << "[StatusServer]Failed to receive message part1" << std::endl;
            //         return;
            //     }
            //     std::cout << "recv " << part.to_string() << std::endl;
            //     parts.push_back(std::move(part));
            //     if (!socket_.get(zmq::sockopt::rcvmore))
            //         break;
            // }

            // if (parts.empty()){
            //     std::cerr << "[StatusServer]Received empty message." << std::endl;
            //     continue;
            // }

            // current_identity_.clear();
            // for (size_t i = 0; i < parts.size() - 1; i++){
            //     current_identity_.push_back(std::move(parts[i]));
            // }

            std::string req_str(static_cast<char*>(data_msg.data()), data_msg.size());
            Json::Reader reader;
            Json::Value value;
            bool parse_success = reader.parse(req_str, value);
            if (!parse_success)
            {
                std::cout << "StatusServer parse json failed" << std::endl;
                continue;
            }
            
            _handler[value["ID"].asString()](value);
        }
        catch(std::exception &e){
            std::cerr << "[StatusServer] Error: " << e.what() << std::endl;
            Json::Value reply;
            reply["error"] = ErrorCodes::InternalError;
            socket_.send(zmq::buffer(reply.toStyledString()), zmq::send_flags::none);
        }
    }
    });
}

void StatusServer::HandleGetChatServer(Json::Value& value)
{
    int uid = value["uid"].asInt();
    try{
        if (uid <= 0){
            Json::Value reply;
            reply["error"] = ErrorCodes::InvalidParam;
            SendReply(reply);
            return;
        }

        ChatServer s = getChatServer();
        std::string token = generate_unique_string();
        std::cout << "create token: " << token << std::endl;
        insertIntoRedis(uid, token);
        insertToken(uid, token);

        Json::Value reply;
        reply["Host"] = s.host;
        reply["Port"] = s.port;
        reply["Token"] = token;
        reply["error"] = ErrorCodes::Success;
        SendReply(reply);
    }
    catch(std::exception &e){
        std::cerr << "[StatusServer] Error: " << e.what() << std::endl;
        Json::Value reply;
        reply["error"] = ErrorCodes::InternalError;
        SendReply(reply);
    }
}

ChatServer StatusServer::getChatServer()
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto minServer = _servers.begin()->second;
    for (const auto &server : _servers)
    {
        if (minServer.con_count > server.second.con_count)
        {
            minServer = server.second;
        }
    }

    return minServer;
}

void StatusServer::insertIntoRedis(int uid, std::string token)
{
    std::string str_uid = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + str_uid;
    RedisMgr::GetInstance()->Set(token_key, token);
}

void StatusServer::insertToken(int uid, std::string token)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _tokens.insert(std::make_pair(uid, token));
}

void StatusServer::HandleVerifyToken(Json::Value& value)
{
    Json::Value reply;
    std::string uid = value["uid"].asString();
    std::string token = value["token"].asString();
    try
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _tokens.find(std::stoi(uid));
        if (it == _tokens.end())
        {
            reply["error"] = ErrorCodes::UidInvalid;
            SendReply(reply);
            return;
        }

        if (it->second != token)
        {
            reply["error"] = ErrorCodes::TokenInvalid;
            SendReply(reply);
            return;
        }

        reply["error"] = ErrorCodes::Success;
        reply["uid"] = uid;
        reply["token"] = token;
        SendReply(reply);
    }
    catch(const std::exception& e)
    {
        reply["error"] = ErrorCodes::InternalError;
        SendReply(reply);
    }
}

void StatusServer::SendReply(const Json::Value& reply)
{
    // for (size_t i = 0; i < current_identity_.size(); i++){
    //     socket_.send(current_identity_[i], zmq::send_flags::sndmore);
    // }

    // zmq::message_t empty_delimiter(0);
    // socket_.send(empty_delimiter, zmq::send_flags::sndmore);

    // std::string reply_str = reply.toStyledString();
    // socket_.send(zmq::buffer(reply_str), zmq::send_flags::none);

    std::string jsonstr = reply.toStyledString();
    try {
		socket_.send(identity_, zmq::send_flags::sndmore);
		socket_.send(zmq::buffer(""), zmq::send_flags::sndmore);
		socket_.send(zmq::buffer(jsonstr), zmq::send_flags::none);
	}
	catch (const std::exception& e) {
		std::cerr << "[VerifyServer] Send error: " << e.what() << std::endl;
	}
}