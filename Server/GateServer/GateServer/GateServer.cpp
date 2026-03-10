#include "const.h"
#include "CServer.h"
#include "CfgManager.h"

int main()
{
	try
	{
		CfgManager gCfgMgr;
		std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
		unsigned short port = static_cast<unsigned short>(atoi(gate_port_str.c_str()));
		boost::asio::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&](const boost::system::error_code& error, int signal_number) {
			if (error)
				return;

			ioc.stop();
			});

		std::make_shared<CServer>(ioc, port)->Start();
		ioc.run();
	}
	catch (std::exception& exp)
	{
		std::cerr << "Error: " << exp.what() << std::endl;
		return EXIT_FAILURE;
	}
}