#pragma once
#include "const.h"

class LogicSystem;
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(boost::asio::ip::tcp::socket socket);

	void Start();

private:
	void CheckDeadLine();
	void WriteResponse();
	void HandleReq();

private:
	boost::asio::ip::tcp::socket _socket;
	boost::beast::flat_buffer _buffer;
	http::request<http::dynamic_body> _request;
	http::response<http::dynamic_body> _response;
	boost::asio::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	};

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};