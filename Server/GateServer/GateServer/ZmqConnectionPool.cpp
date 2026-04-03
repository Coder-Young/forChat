#include "ZmqConnectionPool.h"
#include "CfgManager.h"

ZmqConnectPool::ZmqConnectPool(std::string endpoint, size_t poolSize)
    : poolSize_(poolSize), endpoint_(endpoint), b_stop_(false), context_(1)
{
    for (size_t i = 0; i < poolSize_; i++)
    {
        std::shared_ptr<zmq::socket_t> dealer = std::make_shared<zmq::socket_t>(context_, ZMQ_DEALER);

        dealer->set(zmq::sockopt::rcvtimeo, 5000);
        dealer->set(zmq::sockopt::sndtimeo, 5000);

        try
        {
            dealer->connect(endpoint_);
            connections_.push(dealer);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ZmqConnectPool] Failed to connect socket " << i << ": " << e.what() << std::endl;
        }
    }
}

ZmqConnectPool::~ZmqConnectPool()
{
    Shutdown();
}

void ZmqConnectPool::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        b_stop_ = true;
    }
    cond_.notify_all();
}

std::shared_ptr<zmq::socket_t> ZmqConnectPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() {
			if (b_stop_ || !connections_.empty())
				return true;
            });

    if (b_stop_ || connections_.empty())
        return nullptr;

    auto tmp = connections_.front();
    connections_.pop();
    return tmp;
}

void ZmqConnectPool::returnConnection(std::shared_ptr<zmq::socket_t> &conn_)
{
    if (!conn_)
        return;

    // 清空残留消息
    clearSocketMessages(conn_);
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_)
        return;

    connections_.push(conn_);
    cond_.notify_one();
}

void ZmqConnectPool::clearSocketMessages(std::shared_ptr<zmq::socket_t>& con)
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