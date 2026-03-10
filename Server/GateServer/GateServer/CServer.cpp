#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) : 
	ioc_(ioc),
	acceptor_(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	socket_(ioc)
{
	std::cout << "GateServer start listen at " << port << std::endl;
}

void CServer::Start()
{
	auto self = shared_from_this();
	auto &ioc = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_connection
		= std::make_shared<HttpConnection>(ioc);
	acceptor_.async_accept(new_connection->getSocket(), [new_connection, self](boost::beast::error_code ec) {
		try {
			if (ec) {
				self->Start();
				return;
			}

			new_connection->Start();

			self->Start();
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		});
}