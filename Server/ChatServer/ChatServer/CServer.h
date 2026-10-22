#pragma once
#include "const.h"

class CSession;
class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context&, short port);
    ~CServer();

    void start_accept();
    void ClearSession(std::string);
    bool CheckValid(std::string);

    void on_timer(const boost::system::error_code& e);
	void StartTimer();
	void StopTimer();
private:
    std::mutex _mutex;
    boost::asio::io_context& _ioc;
    boost::asio::ip::tcp::acceptor _acceptor;

    std::map<std::string, std::shared_ptr<CSession>> _sessions;

    boost::asio::steady_timer _timer;
};