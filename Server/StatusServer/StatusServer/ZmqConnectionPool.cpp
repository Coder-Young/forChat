#include "ZmqConnectionPool.h"

ZmqConnectionPool::ZmqConnectionPool(std::string endpoint, size_t poolSize/* = 3*/) :
    m_endpoint(endpoint), m_poolSize(poolSize)
{
    for (size_t i = 0; i < m_poolSize; i++){
        std::shared_ptr<zmq::socket_t> dealer = std::make_shared<zmq::socket_t>(context_, ZMQ_DEALER);

        dealer->set(zmq::sockopt::rcvtimeo, 5000);
        dealer->set(zmq::sockopt::sndtimeo, 5000);

        try{
            dealer->connect(m_endpoint);
            _connections.push(dealer);
        }
        catch(std::exception &e){
            myspdlog::error("[ZmqConnectPool] Failed to connect socket {}: {}", i, e.what());
        }
    }
}

ZmqConnectionPool::~ZmqConnectionPool()
{

}

std::shared_ptr<zmq::socket_t> ZmqConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this](){
        if (!_connections.empty()){
            return true;
        }

        return false;
    });

    auto con = _connections.front();
    _connections.pop();
    return con;
}

void ZmqConnectionPool::returnConnection(std::shared_ptr<zmq::socket_t> con)
{
    assert(con != nullptr);
    if (con == nullptr){
        return;
    }

    std::unique_lock<std::mutex> lock(_mutex);
    clearSocketMsg(con);
    _connections.push(con);
    _cond.notify_one();
}

void ZmqConnectionPool::clearSocketMsg(std::shared_ptr<zmq::socket_t> con)
{
    zmq::message_t can;
    while (true){
        try{
            if (!con->recv(can, zmq::recv_flags::dontwait)){
                break;
            }
        }
        catch(std::exception &e){
            myspdlog::error("[ZmqConnectPool] clear socket msg error: {}", e.what());
        }
    }
}