#include "CServer.h"
#include "CSession.h"
#include "const.h"

CServer::CServer(boost::asio::io_context& ioc, short port)
    : _ioc(ioc), _acceptor(_ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{

}

void CServer::start_accept()
{
    auto self = shared_from_this();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(_ioc);
    _acceptor.async_accept(new_session->getSocket(), [new_session, self](boost::system::error_code error){
        if (error)
        {
            self->start_accept();
            return;
        }
        
        new_session->Start();
        
        self->start_accept();
    });
}