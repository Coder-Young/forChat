#pragma once
#include "const.h"

class ZmqConnectionPool
{
public:
    ZmqConnectionPool(std::string endpoint, size_t poolSize = 3);

    ~ZmqConnectionPool();

    std::shared_ptr<zmq::socket_t> getConnection();
    void returnConnection(std::shared_ptr<zmq::socket_t> con);

private:
    void clearSocketMsg(std::shared_ptr<zmq::socket_t> con);

private:
    zmq::context_t context_;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<std::shared_ptr<zmq::socket_t>> _connections;

    std::string m_endpoint;
    size_t m_poolSize;
};