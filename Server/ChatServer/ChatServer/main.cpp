#include "CServer.h"
#include "const.h"
#include "ConfigManager.h"
#include "RedisMgr.h"

int main()
{
    CfgManager cfg;
    auto server_name = cfg["SelfServer"]["Name"];
    try{
        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

        short port = static_cast<short>(std::stoi(cfg["ChatServer"]["Port"]));
        boost::asio::io_context ioc;
        std::shared_ptr<CServer> g_status_server = std::make_shared<CServer>(ioc, port);

        //启动定时器
        g_status_server->StartTimer();

        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&g_status_server, &ioc](const boost::system::error_code& error, int signal_number){
            if (!error){
                std::cout << "shutting down StatusServer...\n";
                //g_status_server.reset();
                ioc.stop();
            }
        });

        g_status_server->start_accept();
        std::cout << "[ChatServer] Running on " << port << std::endl;
        ioc.run();

        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
    }
    catch(std::exception &e){
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}