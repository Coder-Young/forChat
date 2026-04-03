#include "StatusServer.h"
#include "CfgManager.h"
#include "const.h"

int main()
{
    try{
        CfgManager cfg;
        std::string endpoint = cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"];

        std::unique_ptr<StatusServer> g_status_server = std::make_unique<StatusServer>(endpoint);
        boost::asio::io_context ioc{1};
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& error, int signal_number){
            if (!error){
                std::cout << "shutting down StatusServer...\n";
                g_status_server.reset();
                ioc.stop();
            }
        });
        g_status_server->Start();
        ioc.run();
    }
    catch(std::exception &e){
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

}