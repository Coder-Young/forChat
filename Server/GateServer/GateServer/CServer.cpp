#include "CServer.h"
#include "HttpConnection.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) : 
	ioc_(ioc),
	acceptor_(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	socket_(ioc)
{

}

void CServer::Start()
{
	auto self = shared_from_this();
	acceptor_.async_accept(socket_, [self](boost::beast::error_code ec) {
		try {
			if (ec) {
				self->Start();
				return;
			}

			std::make_shared<HttpConnection>(std::move(self->socket_))->Start();

			self->Start();
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		});
}