#pragma once
#include "ZmqConnectionPool.h"

class ZmqConnectionGuard
{
public:
    ZmqConnectionGuard(ZmqConnectionPool& pool)
        : pool_(pool), connection_(pool.getConnection()) {}

    ~ZmqConnectionGuard(){
        pool_.returnConnection(connection_);
    }

    ZmqConnectionGuard(const ZmqConnectionGuard&) = delete;
    ZmqConnectionGuard& operator=(const ZmqConnectionGuard&) = delete;

    std::shared_ptr<zmq::socket_t> get(){
        return connection_;
    }

private:
    std::shared_ptr<zmq::socket_t> connection_;
    ZmqConnectionPool& pool_;
};