#pragma once

#include "const.h"
#include "Singleton.h"
#include "MsgNode.h"

class LogicNode;
class CSession;
typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& body)> MsgHandler;

class LogicSystem : public Singleton<LogicSystem>
{
    friend Singleton<LogicSystem>;
public:
    ~LogicSystem();

    void PostToMsgQueue(std::shared_ptr<LogicNode>);
    void RegisterHandler();

private:
    void work();

    void LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& body);

private:
    LogicSystem(size_t thread_num = std::thread::hardware_concurrency());

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<std::shared_ptr<LogicNode>> _msg_queue;
    std::unordered_map<short, MsgHandler> _handler_map;
    std::unordered_map<int, std::shared_ptr<UserInfo>> _users_map;
    std::vector<std::thread> _worker_thread;

    std::atomic<bool> m_stop;
};