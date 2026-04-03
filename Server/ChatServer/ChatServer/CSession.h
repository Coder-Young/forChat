#pragma once
#include "const.h"
#include "MsgNode.h"

typedef std::function<void(boost::system::error_code &, size_t)> handlerFunc;

class LogicSystem;
class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& ioc);

    boost::asio::ip::tcp::socket& getSocket() { return _socket; }

    void Start();
    void Close();
    void Send(char* msg,  short max_length, short msgid);
	void Send(std::string msg, short msgid);

private:
    void AsyncReadHead(int);
    void AsyncReadBody(int);

    void HandleReadFull(int, handlerFunc);
    void HandleReadLen(int, int, handlerFunc);

    void HandleWrite(boost::system::error_code err, std::size_t bytes_writed, std::shared_ptr<CSession> shared_self);

private:
    boost::asio::ip::tcp::socket _socket;
    std::mutex _mutex;
    std::mutex _send_lock;

    enum{
        max_length = 1024
    };
    char _data[max_length];

    bool b_pending;
    std::queue<std::shared_ptr<SendNode>> _send_queue;
    std::shared_ptr<MsgNode> _msg_head;
    std::shared_ptr<RecvNode> _msg_node;
    std::shared_ptr<LogicNode> _logic_node;
};

class LogicNode
{
    friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
        : _session(session), _recvnode(recvnode)
        {}

private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};