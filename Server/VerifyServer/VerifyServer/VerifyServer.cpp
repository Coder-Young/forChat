#include "VerifyServer.h"
#include "EmailService.h"
#include <iostream>
#include <curl/curl.h>

VerifyServer::VerifyServer(const std::string& addr, int threadNum) :
	context_(1), socket_(context_, ZMQ_ROUTER), b_stop(false), _threadNum(threadNum)
{
	socket_.bind(addr);
	std::cout << "[VerifyServer] Listening on " << addr << std::endl;
}

VerifyServer::~VerifyServer()
{
	Stop();
}

void VerifyServer::Stop()
{
	b_stop = true;

	_queue_state.notify_all();

	for (auto& t : _threads)
	{
		if (t.joinable()) {
			t.join();
		}
	}
}

void VerifyServer::Run()
{
	_threads.reserve(_threadNum);
	for (int i = 0; i < _threadNum; i++)
	{
		_threads.emplace_back(&VerifyServer::threadWorker, this);
	}

	std::cout << "[VerifyServer] Server started, waiting for requests..." << std::endl;
	while (!b_stop)
	{
		try {
			zmq::message_t identity_msg;
			auto id_res = socket_.recv(identity_msg, zmq::recv_flags::none);
			if (!id_res.has_value())
				continue;

			zmq::message_t data_msg;
			auto data_res = socket_.recv(data_msg, zmq::recv_flags::none);
			if (!data_res.has_value())
				continue;

			std::string json_str(static_cast<char*>(data_msg.data()), data_msg.size());
			Json::Reader reader;
			Json::Value res_data;
			bool parse_res = reader.parse(json_str, res_data);
			if (!parse_res)
			{
				SendError(identity_msg, "Json parse error");
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(_queue_lock);
				EmailRequest eq(std::move(identity_msg), std::move(res_data));
				_request_queue.push(std::move(eq));
				_queue_state.notify_one();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "[VerifyServer] Exception: " << e.what() << std::endl;
		}
	}
}

void VerifyServer::threadWorker()
{
	while (!b_stop)
	{
		EmailRequest req;
		{
			std::unique_lock<std::mutex> lock(_queue_lock);

			_queue_state.wait(lock, [this]() {
				return b_stop || !_request_queue.empty();
				});

			if (b_stop && _request_queue.empty())
				break;

			req = std::move(_request_queue.front());
			_request_queue.pop();
		}

		HandleRequest(std::move(req));
	}
}

void VerifyServer::HandleRequest(EmailRequest&& req)
{
	//SendResponse
	if (!req.request.isMember("email") || !req.request["email"].isString())
	{
		SendError(req.identity, "need email");
		return;
	}

	std::string email = req.request["email"].asString();
	std::string code = GenerateVerifyCode();
	std::cout << "[VerifyServer] Generated code: " << code << " for " << email << std::endl;

	std::string title = "VerifyCode";
	std::string body =
		"Hello,\n\n"
		"You are requesting a verification code.\n"
		"Your code is: " + code + "\n"
		"This code is valid for 5 minutes.\n\n"
		"Best regards,\n"
		"ForChat Team";
	//std::string body = fmt::format("���ã�\n\n������֤���ǣ�{}\n\n����֤�� 5 ��������Ч��\n\nVerifyServer �Ŷ�", code);
	EmailResult res = EmailService::GetInstance()->SendEmail(email, title, body);
	
	Json::Value response;
	if (res.success) {
		response["status"] = "ok";
		response["msg"] = code;
		SendResponse(req.identity, response);
	}
	else
	{
		response["status"] = "error";
		response["msg"] = res.message;
		SendResponse(req.identity, response);
	}
}

void VerifyServer::SendError(zmq::message_t& identity, const std::string& msg)
{
	Json::Value err;
	err["status"] = "error";
	err["msg"] = msg;
	SendResponse(identity, err);
}

void VerifyServer::SendResponse(zmq::message_t& identity, const Json::Value& response)
{
	std::string json_str = response.toStyledString();
	std::lock_guard<std::mutex> lock(zmq_socket_lock);
	try {
		socket_.send(identity, zmq::send_flags::sndmore);
		socket_.send(zmq::buffer(""), zmq::send_flags::sndmore);
		socket_.send(zmq::buffer(json_str), zmq::send_flags::none);
	}
	catch (const std::exception& e) {
		std::cerr << "[VerifyServer] Send error: " << e.what() << std::endl;
	}
}