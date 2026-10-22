#include "LogicSystem.h"
#include "StatusZmqClient.h"
#include "ChatZmqClient.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"
#include "ConfigManager.h"
#include "UserMgr.h"

LogicSystem::LogicSystem(size_t thread_num)
    : m_stop(false), _p_server(nullptr)
{
    RegisterHandler();

    for (int i = 0; i < thread_num; i++)
    {
        _worker_thread.emplace_back(std::thread(&LogicSystem::work, this));
    }
}

LogicSystem::~LogicSystem()
{
    m_stop = true;

    for (int i = 0; i < _worker_thread.size(); i++){
        if (_worker_thread[i].joinable()){
            _worker_thread[i].join();
        }
    }
}

void LogicSystem::work()
{
    while (true)
    {
        std::unique_lock<std::mutex> unique_lk(_mutex);
        _cond.wait(unique_lk, [this](){
            if (!_msg_queue.empty() || m_stop)
                return true;
			return false;
        });

        if (m_stop)
        {
            while (!_msg_queue.empty()){
                std::shared_ptr<LogicNode> node = _msg_queue.front();
                auto it = _handler_map.find(node->_recvnode->msg_id);
                if (it == _handler_map.end())
                {
                    _msg_queue.pop();
                    std::cout << "[LogicSystem]no handler function" << std::endl;
                    continue;
                }

                it->second(node->_session, node->_recvnode->msg_id, std::string(node->_recvnode->data, node->_recvnode->cur_len));
                _msg_queue.pop();
            }
            break;
        }

        std::shared_ptr<LogicNode> node = _msg_queue.front();
        auto it = _handler_map.find(node->_recvnode->msg_id);
        if (it == _handler_map.end())
        {
            _msg_queue.pop();
            std::cout << "[LogicSystem]no handler function" << std::endl;
            continue;
        }

        it->second(node->_session, node->_recvnode->msg_id, std::string(node->_recvnode->data, node->_recvnode->cur_len));
        _msg_queue.pop();
    }
}

void LogicSystem::PostToMsgQueue(std::shared_ptr<LogicNode> msg_node)
{
    std::unique_lock<std::mutex> lock_guard(_mutex);
    _msg_queue.push(msg_node);
    if (_msg_queue.size() >= 1)
    {
        lock_guard.unlock();
        _cond.notify_one();
    }
}

void LogicSystem::RegisterHandler()
{
	// 登录请求
    _handler_map.insert(std::make_pair(ID_CHAT_LOGIN_REQ, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->LoginHandler(session, msg_id, body);
    }));

	// 好友查询
    _handler_map.insert(std::make_pair(ID_SEARCH_USER_REQ, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->SearchInfoHandler(session, msg_id, body);
    }));

	// 添加好友
    _handler_map.insert(std::make_pair(ID_ADD_FRIEND_REQ, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->AddFriendHandler(session, msg_id, body);
    }));

	// 回复添加好友请求
    _handler_map.insert(std::make_pair(ID_AUTH_FRIEND_REQ, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->AcceptFriendHandler(session, msg_id, body);
    }));

	// 消息
    _handler_map.insert(std::make_pair(ID_TEXT_CHAT_MSG_REQ, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->DealChatTextMsgHandler(session, msg_id, body);
    }));

	// 心跳
    _handler_map.insert(std::make_pair(ID_HEART_BEAT_REQ, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->HeartBeatHandler(session, msg_id, body);
    }));
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(body, root);

    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();
    //bool res = StatusZmqClient::GetInstance()->VerifyToken(uid, token);

    Json::Value rtvalue;
    Defer defer([this, &rtvalue, session](){
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, ID_CHAT_LOGIN_RSP);
    });

    // if (!res){
    //     rtvalue["error"] = ErrorCodes::VerifyFailed;
    //     return;
    // }

    std::string str_uid = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + str_uid;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    if (token_value != token)
    {
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }

    rtvalue["error"] = ErrorCodes::Success;
	std::string base_key = USERBASEINFO + str_uid;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = GetBaseInfo(base_key, uid, user_info);
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

    // 获得好友申请列表
	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid, apply_list);
	if (b_apply) {
		for (auto& apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["icon"] = apply->_icon;
			obj["nick"] = apply->_nick;
			obj["sex"] = apply->_sex;
			obj["desc"] = apply->_desc;
			obj["status"] = apply->_status;
			rtvalue["apply_list"].append(obj);
		}
	}

    // 获得好友列表
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		rtvalue["friend_list"].append(obj);
	}
	
    CfgManager cfg;
    auto server_name = cfg["SelfServer"]["Name"];
	{
		auto lock_key = LOCK_PREFIX + str_uid;
		auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);

		Defer defer2([this, identifier, lock_key]() {
			RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
		});

		std::string uid_ip_value = "";
		auto uid_ip_key = USERIPPERFIX + str_uid;
		bool b_ip = RedisMgr::GetInstance()->Get(uid_ip_key, uid_ip_value);
		// 用户已登录
		if (b_ip) {
			auto self_name = cfg["SelfServer"]["Name"];
			if (uid_ip_value == self_name) {
				// 本服踢
				auto old_session = UserMgr::GetInstance()->GetSession(uid);
				if (old_session) {
					//old_session->NotifyOffline(uid); 发送踢人消息
					_p_server->ClearSession(old_session->GetSessionId());
				}
			}
			else {
				// 跨服踢
				Json::Value value;
				value["uid"] = uid;
				ChatZmqClient::GetInstance()->NotifyKickUser(uid_ip_value, value);
			}
		}
	}

	// 增加登录数量
	auto res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!res.empty()){
        count = std::stoi(res);
    }

    auto count_str = std::to_string(++count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);

	// session绑定用户uid
	session->SetUserId(uid);
	std::string ipkey = USERIPPERFIX + str_uid;
	RedisMgr::GetInstance()->Set(ipkey, server_name);

    UserMgr::GetInstance()->SetUserSession(uid, session);
	std::string uid_session_key = USER_SESSION_PREFIX + str_uid;
	RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());

	return;

}

void LogicSystem::SearchInfoHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body)
{
    Json::Reader reader;
	Json::Value root;
	bool parse_success = reader.parse(body, root);
    if (!parse_success){

    }

	auto uid_str = root["uid"].asString();

	Json::Value  rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	bool b_digit = isPureDigit(uid_str);
	if (b_digit) {
		GetUserByUid(uid_str, rtvalue);
	}
	else {
		GetUserByName(uid_str, rtvalue);
	}
	return;
}

void LogicSystem::AddFriendHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body)
{
	Json::Reader reader;
	Json::Value root;
	bool parse_success = reader.parse(body, root);
    if (!parse_success){
        return;
    }

	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_ADD_FRIEND_RSP);
		});

    bool sql_success = MySqlMgr::GetInstance()->AddFriendApply(uid, touid);
    if (!sql_success){
        return;
    }

	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPERFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	std::string base_key = USERBASEINFO + std::to_string(uid);
	auto apply_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, uid, apply_info);

    CfgManager cfg;
	auto self_name = cfg["SelfServer"]["Name"];
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			notify["desc"] = "";
			if (b_info) {
				notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
			}
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}
		return;
	}

    Json::Value req;
    req["applyuid"] = uid;
    req["touid"] = touid;
    req["name"] = applyname;
    req["desc"] = "";
    if (b_info){
        req["icon"] = apply_info->icon;
        req["sex"] = apply_info->sex;
        req["nick"] = apply_info->nick;
    }

    ChatZmqClient::GetInstance()->NotifyAddFriend(to_ip_value, req);
}

void LogicSystem::AcceptFriendHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body)
{
    Json::Reader reader;
	Json::Value root;
	bool parse_success = reader.parse(body, root);
    if (!parse_success){
        return;
    }

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();

	std::string base_key = USERBASEINFO + std::to_string(touid);
	bool b_info = GetBaseInfo(base_key, touid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
		rtvalue["uid"] = touid;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);
		});

    MySqlMgr::GetInstance()->AcceptFriendApply(uid, touid);
    MySqlMgr::GetInstance()->AddFriend(uid, touid, back_name);

	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPERFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

    CfgManager cfg;
	auto self_name = cfg["SelfServer"]["Name"];
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			Json::Value notify;
			notify["error"] = ErrorCodes::Success;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USERBASEINFO + std::to_string(uid);
			auto user_info = std::make_shared<UserInfo>();
			bool b_info = GetBaseInfo(base_key, uid, user_info);
			if (b_info) {
				notify["name"] = user_info->name;
				notify["nick"] = user_info->nick;
				notify["icon"] = user_info->icon;
				notify["sex"] = user_info->sex;
			}
			else {
				notify["error"] = ErrorCodes::UidInvalid;
			}

			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}

		return ;
	}

    Json::Value request;
    request["fromuid"] = uid;
    request["touid"] = touid;
    ChatZmqClient::GetInstance()->NotifyAcceptFriend(to_ip_value, request);
}

void LogicSystem::DealChatTextMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
	Json::Reader reader;
	Json::Value root;
	reader.parse(body, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	const Json::Value  arrays = root["text_array"];

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
	});

	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPERFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	CfgManager cfg;
	auto self_name = cfg["SelfServer"]["Name"];
	if (to_ip_value == self_name) {
		// 同服务器
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}
		return;
	}

	// todo
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
	Json::Reader reader;
	Json::Value root;
	reader.parse(body, root);
	auto uid = root["fromuid"].asInt();
	std::cout << "receive heart beat msg, uid is " << uid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
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

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = NAME_INFO + name;
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}

	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MySqlMgr::GetInstance()->GetUser(name);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;
	std::string base_key = USERBASEINFO + uid_str;

	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
	}

	auto uid = std::stoi(uid_str);
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MySqlMgr::GetInstance()->GetUser(uid);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	redis_root["icon"] = user_info->icon;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;
}

bool LogicSystem::isPureDigit(const std::string& str)
{
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>> &list) {
	return MySqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list) {
	return MySqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}