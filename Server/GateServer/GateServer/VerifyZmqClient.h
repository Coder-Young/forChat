#pragma once
#include "const.h"
#include "CfgManager.h"
#include <zmq.hpp>

class ZmqConnectPool
{
public:
	ZmqConnectPool(size_t poolSize) :
		poolSize_(poolSize), b_stop_(false), context_(1)
		{
			CfgManager gCfgMgr;
    		std::string endpoint_ = gCfgMgr["VerifyServer"]["Address"] + ":" + gCfgMgr["VerifyServer"]["Port"];

			for (size_t i = 0; i < poolSize_; i++)
			{
				std::shared_ptr<zmq::socket_t> dealer 
					= std::make_shared<zmq::socket_t>(context_, ZMQ_DEALER);

				dealer->set(zmq::sockopt::rcvtimeo, 5000);
				dealer->set(zmq::sockopt::sndtimeo, 5000);

				try {
					dealer->connect(endpoint_);
					connections_.push(dealer);
				}
				catch(const std::exception& e){
					std::cerr << "[ZmqConnectPool] Failed to connect socket " << i << ": " << e.what() << std::endl;
				}
			}
		}

	~ZmqConnectPool()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty())
		{
			connections_.pop();
		}
	}

	void Close()
	{
		b_stop_ = true;
		cond_.notify_all();
	}

	std::shared_ptr<zmq::socket_t> getConnection()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this](){
			if (b_stop_ || !connections_.empty())
				return true;
		});

		if (b_stop_ || connections_.empty())
			return nullptr;

		auto tmp = connections_.front();
		connections_.pop();
		return tmp;
	}

	void returnConnection(std::shared_ptr<zmq::socket_t>& conn_)
	{
		if (!conn_)
			return;

		// 清空残留消息
		clearSocketMessages(*conn_);
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_)
			return;

		connections_.push(conn_);
		cond_.notify_one();
	}

private:
		void clearSocketMessages(zmq::socket_t& socket)
	{
    	socket.set(zmq::sockopt::rcvtimeo, 0);
    	zmq::message_t msg;
    	while (true) {
        	auto res = socket.recv(msg, zmq::recv_flags::none);
        	if (!res.has_value()) {
            	break;
        	}
    	}
    	socket.set(zmq::sockopt::rcvtimeo, 5000);
	}
private:
	zmq::context_t context_;
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::queue<std::shared_ptr<zmq::socket_t>> connections_;
	std::condition_variable cond_;
	std::mutex mutex_;
};

class VerifyZmqClient : public Singleton<VerifyZmqClient>
{
	friend Singleton<VerifyZmqClient>;
public:
	VerifyZmqClient();
	~VerifyZmqClient();

	std::string RequestVerifyCode(const std::string& email);

private:
	bool SendAndRecvMessage(const Json::Value& request, Json::Value& response);

private:
	std::unique_ptr<ZmqConnectPool> zmqConnPool;
};