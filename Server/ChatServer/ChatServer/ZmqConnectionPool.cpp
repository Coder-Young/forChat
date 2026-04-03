#include "ZmqConnectionPool.h"

ZmqConnectionPool::ZmqConnectionPool(std::string endpoint, size_t poolSize/* = 3*/)
    : endpoint_(endpoint), pool_size(3)
{
    for (int i = 0; i < pool_size; i++)
    {
        std::shared_ptr<zmq::socket_t> dealer = std::make_shared<zmq::socket_t>(context_, ZMQ_DEALER);

        dealer->set(zmq::sockopt::rcvtimeo, 5000);
        dealer->set(zmq::sockopt::sndtimeo, 5000);

        try
        {
            dealer->connect(endpoint_);
            connections_.push(dealer);
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ZmqConnectPool] Failed to connect socket " << i << ": " << e.what() << std::endl;
        }
    }
}

ZmqConnectionPool::~ZmqConnectionPool()
{

}

std::shared_ptr<zmq::socket_t> ZmqConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> unique_lk(mutex_);
    cond_.wait(unique_lk, [this](){
        if (b_stop || !connections_.empty()){
            return true;
        }
        return false;
    });

    if (b_stop)
    {
        return nullptr;
    }

    auto con = connections_.front();
    connections_.pop();
    return con;
}

void ZmqConnectionPool::returnConnection(std::shared_ptr<zmq::socket_t>& con)
{
    if (!con)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop)
        return;
    
    clearSocketMsg(con);
    connections_.push(con);
    cond_.notify_one();
}

void ZmqConnectionPool::clearSocketMsg(std::shared_ptr<zmq::socket_t>& con)
{
    zmq::message_t message;
    while (true) {
        try {
            if (!con->recv(message, zmq::recv_flags::dontwait)) {
                break;
            }
        } catch (const zmq::error_t& e) {
            if (e.num() != EAGAIN && e.num() != EINTR) {
                std::cerr << "Error while clearing socket: " << e.what() << "\n";
            }
            break;
        }
    }
}