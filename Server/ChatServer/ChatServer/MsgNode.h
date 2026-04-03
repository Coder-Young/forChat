#pragma once
#include <cstring>
#include "LogicSystem.h"

class MsgNode
{
public:
    MsgNode(short _max_len) : cur_len(0), max_len(_max_len)
    {
        data = new char[max_len + 1];
        data[max_len] = '\0';
    }

    ~MsgNode(){
        if (data != nullptr){
            delete [] data;
            data = nullptr;
        }
    }

    void Clear(){
        memset(data, 0, max_len);
        cur_len = 0;
    }

    char* data;
    short cur_len;
    short max_len;
};

class RecvNode : public MsgNode
{
    friend class LogicSystem;
public:
    RecvNode(short _msg_id, short _max_len) : msg_id(_msg_id), MsgNode(_max_len)
    {}

private:
    short msg_id;
};

class SendNode : public MsgNode
{
    friend class LogicSystem;
public:
    SendNode(const char* msg, short _msg_id, short _max_len)
        : msg_id(_msg_id), MsgNode(_max_len)
    {
        memset(data, 0, max_len);
        short net_msg_id = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
        memcpy(data, &net_msg_id, HEAD_ID_LEN);
        short net_max_len = boost::asio::detail::socket_ops::host_to_network_short(max_len);
        memcpy(data + HEAD_ID_LEN, &net_max_len, HEAD_DATA_LEN);
        memcpy(data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
    }


private:
    short msg_id;
};