#pragma once
#include "const.h"
#include "Singleton.h"

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
	friend Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using WorkGuard = boost::asio::executor_work_guard<IOService::executor_type>;
	using WorkGuardPtr = std::unique_ptr<WorkGuard>;

	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
	~AsioIOServicePool();

	IOService& GetIOService();
	void Stop();

private:
	AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());

	std::vector<IOService> _ioServices;
	std::vector<WorkGuardPtr> _work_guards;
	std::vector<std::thread> _threads;
	std::size_t _nextIOServiceIndex;
};
