#pragma once
#include "const.h"

class ZmqConnectPool
{
public:
    ZmqConnectPool(std::string endpoint, size_t poolSize = 3);
    ~ZmqConnectPool();
	void Shutdown();

	std::shared_ptr<zmq::socket_t> getConnection();
	void returnConnection(std::shared_ptr<zmq::socket_t>& conn_);

private:
	void clearSocketMessages(std::shared_ptr<zmq::socket_t>& con);

private:
	zmq::context_t context_;
	std::atomic<bool> b_stop_;
	size_t poolSize_;
    std::string endpoint_;
	std::queue<std::shared_ptr<zmq::socket_t>> connections_;
	std::condition_variable cond_;
	std::mutex mutex_;
};
