#include "ZmqConnectionPool.h"

class ZmqConnectionGuard
{
public:
    ZmqConnectionGuard(ZmqConnectionPool& pool) : 
        m_pool(pool), _conn(m_pool.getConnection()) {}

    ~ZmqConnectionGuard(){
        m_pool.returnConnection(_conn);
    }

    ZmqConnectionGuard(const ZmqConnectionGuard&) = delete;
    ZmqConnectionGuard& operator=(const ZmqConnectionGuard&) = delete;

    std::shared_ptr<zmq::socket_t> get(){
        return _conn;
    }

private:
    ZmqConnectionPool &m_pool;
    std::shared_ptr<zmq::socket_t> _conn;
};