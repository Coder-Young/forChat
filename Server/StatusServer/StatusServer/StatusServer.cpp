#include "StatusServer.h"
#include "CfgManager.h"
#include "const.h"
#include <string>
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
    auto server_lists = cfg["ChatServers"]["ServerName"];
    
    std::vector<std::string> names;
    std::stringstream ss(server_lists);
    std::string words;
    while (std::getline(ss, words, ',')){
        names.push_back(words);
    }

    for (size_t i = 0; i < names.size(); i++){
        ChatServer s;
        s.port = cfg[names[i]]["Port"];
        s.host = cfg[names[i]]["Host"];
        s.name = cfg[names[i]]["Name"];
        s.con_count = 0;
        _servers.insert(std::make_pair(s.name, s));
    }

    try
    {
        socket_.bind(bind_endpoint_);
    }
    catch(const zmq::error_t& e)
    {
        myspdlog::error("ZMQ bind error: {}  (errno: {})", e.what(), e.num());
    }

    myspdlog::info("[StatusServer]Bind to {}", bind_endpoint_);

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
                myspdlog::error("[StatusServer] parse json {} failed", req_str);
                continue;
            }
            
            auto it = _handler.find(value["ID"].asString());
            if (it != handler.end()){
                it->second(value);
            }
            else{
                myspdlog::error("[StatusServer] bad_function_call");
            }
        }
        catch(std::exception &e){
            myspdlog::error("[StatusServer] Error: {}", e.what());
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
        myspdlog::info("[StatusServer]create token: {}", token);
        insertIntoRedis(uid, token);

        Json::Value reply;
        reply["Host"] = s.host;
        reply["Port"] = s.port;
        reply["Token"] = token;
        reply["error"] = ErrorCodes::Success;
        SendReply(reply);
    }
    catch(std::exception &e){
        myspdlog::error("[StatusServer] Error: {}", e.what());
        Json::Value reply;
        reply["error"] = ErrorCodes::InternalError;
        SendReply(reply);
    }
}

ChatServer StatusServer::getChatServer()
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto minServer = _servers.begin()->second;
    
    auto lock_key = LOCK_COUNT;
    auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);

    Defer defer2([this, identifier, lock_key]() {
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
    });

    auto reg_count = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
    if (reg_count.empty()){
        minServer.con_count = INT32_MAX;
    }
    else{
        minServer.con_count = std::stoi(reg_count);
    }

    for (auto &server : _servers)
    {
        if (server.second.name == minServer.name)
            continue;

        auto ot_reg_count = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
        if (ot_reg_count.empty()){
            server.second.con_count = INT32_MAX;
        }
        else{
            server.second.con_count = std::stoi(ot_reg_count);
        }

        if (server.second.con_count < minServer.con_count){
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

void StatusServer::HandleVerifyToken(Json::Value& value)
{
    Json::Value reply;
    std::string uid = value["uid"].asString();
    std::string token = value["token"].asString();
    try
    {
        std::string token_key = USERTOKENPREFIX + uid;
        std::string token_value = "";
        bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
        if (!success){
            reply["error"] = ErrorCodes::UidInvalid;
            SendReply(reply);
            return;
        }

        if (token_value != token){
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
        myspdlog::error("[VerifyServer] Send error: {}", e.what());
	}
}