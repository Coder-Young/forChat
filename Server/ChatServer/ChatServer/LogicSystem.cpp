#include "LogicSystem.h"
#include "StatusZmqClient.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"

LogicSystem::LogicSystem(size_t thread_num)
    : m_stop(false)
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
    _handler_map.insert(std::make_pair(MSG_CHAT_LOGIN, [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& body){
        this->LoginHandler(session, msg_id, body);
    }));
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(body, root);

    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();
    bool res = StatusZmqClient::GetInstance()->VerifyToken(uid, token);

    Json::Value rtvalue;
    if (!res){
        rtvalue["error"] = ErrorCodes::VerifyFailed;
        return;
    }

    Defer defer([this, &rtvalue, session](){
        std::string jsonstr = rtvalue.toStyledString();
        session->Send(jsonstr, MSG_CHAT_LOGIN_RSP);
    });

    // std::string str_uid = std::to_string(uid);
    // std::string token_key = USERTOKENPREFIX + str_uid;
    // std::string token_value = "";
    // bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    // if (!success)
    // {
    //     rtvalue["error"] = ErrorCodes::UidInvalid;
    //     return;
    // }

    // if (token_value != token)
    // {
    //     rtvalue["error"] = ErrorCodes::TokenInvalid;
    //     return;
    // }

    // rtvalue["error"] = ErrorCodes::Success;
    auto iter = _users_map.find(uid);
    std::shared_ptr<UserInfo> user_info = nullptr;
    if (iter == _users_map.end()){
        user_info = MySqlMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr){
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }

        _users_map[uid] = user_info;
    }
    else{
        user_info = iter->second;
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = user_info->name;
}