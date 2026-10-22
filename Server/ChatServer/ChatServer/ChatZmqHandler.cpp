#include "ChatZmqHandler.h"
#include "UserMgr.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"
#include "CSession.h"
#include "CServer.h"

ChatZmqHandler::ChatZmqHandler(const std::string& endpoint) :
    context_(1), socket_(context_, ZMQ_ROUTER), endpoint_(endpoint)
{
    try{
        socket_.bind(endpoint_);
    }
    catch(const zmq::error_t& e){
        std::cerr << "ZMQ bind error: " << e.what() 
        << " (errno: " << e.num() << ")" << std::endl;
    }

    registerHandler();
    std::cout << "ChatServer listen at" << endpoint_ << std::endl;
}

void ChatZmqHandler::registerHandler()
{
    _handlers.insert(std::make_pair(MSG_ID::ID_ADD_FRIEND_REQ, 
        std::bind(&ChatZmqHandler::HandleAddFriend, this, std::placeholders::_1)));

    _handlers.insert(std::make_pair(MSG_ID::ID_AUTH_FRIEND_REQ, 
        std::bind(&ChatZmqHandler::HandleAcceptFriend, this, std::placeholders::_1)));

	_handlers.insert(std::make_pair(MSG_ID::ID_NOTIFY_OFF_LINE_REQ, 
        std::bind(&ChatZmqHandler::HandleKickUser, this, std::placeholders::_1)));
}

void ChatZmqHandler::StartHandling()
{
    std::thread([this](){
        while(true){
            try{
                std::vector<std::string> parts;
                while (true){
                    zmq::message_t part;
                    if (!socket_.recv(part))
                    {
                        std::cerr << "[ChatZmqHandler] Failed to receive request." << std::endl;
                        return false;
                    }

                    parts.emplace_back(static_cast<char*>(part.data()), part.size());
                    if (!socket_.get(zmq::sockopt::rcvmore))
                        break;
                }

                Json::Value request;
                std::string res_str = parts.back();
                Json::Reader reader;
                bool parse_success = reader.parse(res_str, request);
                if (!parse_success){
                    std::cerr << "[ChatZmqHandler] Failed to parse request" << std::endl;
                    return false;
                }

                _handlers[request["MsgID"].asInt()](request);
            }
            catch(std::exception &e){

            }
        }
    }).detach();
}

void ChatZmqHandler::HandleAddFriend(const Json::Value& request)
{
    auto touid = request["touid"].asInt();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Json::Value reply;
	Defer defer([&]() {
        reply["error"] = ErrorCodes::Success;
        reply["touid"] = touid;
        reply["uid"] = request["uid"].asInt();
		});

    if (session == nullptr){
        return;
    }

    Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["applyuid"] = request["applyuid"].asInt();
	rtvalue["name"] = request["name"].asString();
	rtvalue["desc"] = request["desc"].asString();
	rtvalue["icon"] = request["icon"].asString();
	rtvalue["sex"] = request["sex"].asInt();
	rtvalue["nick"] = request["nick"].asString();
    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, MSG_ID::ID_NOTIFY_ADD_FRIEND_REQ);
}

void ChatZmqHandler::HandleAcceptFriend(const Json::Value& request)
{
    auto touid = request["touid"].asInt();
    auto fromuid = request["fromuid"].asInt();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Json::Value reply;
	Defer defer([&]() {
        reply["error"] = ErrorCodes::Success;
        reply["touid"] = touid;
        reply["fromuid"] = fromuid;
		});

    if (session == nullptr) {
		return;
	}

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = fromuid;
	rtvalue["touid"] = touid;

	std::string base_key = USERBASEINFO + std::to_string(fromuid);
	auto user_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, fromuid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}

	std::string return_str = rtvalue.toStyledString();
	session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
}

void ChatZmqHandler::HandleKickUser(const Json::Value& request)
{
	auto uid = request["uid"].asInt();
	auto session = UserMgr::GetInstance()->GetSession(uid);

	Json::Value rtvalue;
    Defer defer([&]() {
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["uid"] = uid;
    });

	if (session == nullptr){
		return;
	}

	session->NotifyOffline(uid);
	_p_server->ClearSession(session->GetSessionId());
}

bool ChatZmqHandler::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
	}
	else {
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MySqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}
	
	return true;
}

void ChatZmqHandler::SendReply(const Json::Value& reply)
{
    std::string jsonstr = reply.toStyledString();
    try {
		socket_.send(identity_, zmq::send_flags::sndmore);
		socket_.send(zmq::buffer(""), zmq::send_flags::sndmore);
		socket_.send(zmq::buffer(jsonstr), zmq::send_flags::none);
	}
	catch (const std::exception& e) {
		std::cerr << "[ChatZmqHandler]Send error: " << e.what() << std::endl;
	}
}