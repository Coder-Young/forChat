#pragma once
#include "const.h"

struct EmailRequest
{
	EmailRequest() = default;
	EmailRequest(const EmailRequest&) = delete;
	EmailRequest& operator=(const EmailRequest&) = delete;

	EmailRequest(EmailRequest&&) noexcept = default;
	EmailRequest(zmq::message_t&& id, Json::Value&& _request)
		: identity(std::move(id)), request(std::move(_request))
	{}

	EmailRequest& operator=(EmailRequest&&) noexcept = default;

	zmq::message_t identity;
	Json::Value request;
};

class VerifyServer
{
public:
	VerifyServer(const std::string& addr, int threadNum = std::thread::hardware_concurrency());
	~VerifyServer();

	void Run();

private:
	void Stop();

	void threadWorker();

	void HandleRequest(EmailRequest&&);

	void SendError(zmq::message_t& identity, const std::string& msg);
	void SendResponse(zmq::message_t& identity, const Json::Value& response);

	bool StoreVerifyCodeRaw(const std::string& email, const std::string& code);

	std::string GenerateVerifyCode() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1000, 9999);
		return std::to_string(dis(gen));
	}

private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::mutex zmq_socket_lock;

	bool b_stop;
	int _threadNum;
	std::mutex _queue_lock;
	std::condition_variable _queue_state;
	std::vector<std::thread> _threads;
	std::queue<EmailRequest> _request_queue;
};