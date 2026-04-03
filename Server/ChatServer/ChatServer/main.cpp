#include "CServer.h"
#include "const.h"
#include "ConfigManager.h"

int main()
{
    try{
        CfgManager cfg;
        short port = static_cast<short>(std::stoi(cfg["ChatServer1"]["Port"]));
        boost::asio::io_context ioc;
        std::shared_ptr<CServer> g_status_server = std::make_shared<CServer>(ioc, port);
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&g_status_server](const boost::system::error_code& error, int signal_number){
            if (!error){
                std::cout << "shutting down StatusServer...\n";
                g_status_server.reset();
            }
        });

        g_status_server->start_accept();
        std::cout << "[ChatServer] Running on " << port << std::endl;
        ioc.run();
    }
    catch(std::exception &e){
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}