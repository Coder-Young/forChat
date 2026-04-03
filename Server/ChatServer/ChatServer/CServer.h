#pragma once
#include "const.h"

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context&, short port);

    void start_accept();
private:
    boost::asio::io_context& _ioc;
    boost::asio::ip::tcp::acceptor _acceptor;
};