#pragma once
#include "const.h"

class ZmqConnectionPool
{
public:
    ZmqConnectionPool(std::string endpoint, size_t poolSize = 3);
    ~ZmqConnectionPool();

    std::shared_ptr<zmq::socket_t> getConnection();
    void returnConnection(std::shared_ptr<zmq::socket_t>& con);

private:
    void clearSocketMsg(std::shared_ptr<zmq::socket_t>& con);

private:
    zmq::context_t context_;
    std::atomic<bool> b_stop;
    size_t pool_size;
    std::string endpoint_;
    std::queue<std::shared_ptr<zmq::socket_t>> connections_;
    std::condition_variable cond_;
    std::mutex mutex_;
};