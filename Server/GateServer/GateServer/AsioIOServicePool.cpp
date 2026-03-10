#include "AsioIOServicePool.h"

AsioIOServicePool::AsioIOServicePool(std::size_t size/* = std::thread::hardware_concurrency()*/) :
	_ioServices(size), _work_guards(size), _threads(size), _nextIOServiceIndex(0)
{
	for (std::size_t i = 0; i < size; i++)
	{
		_work_guards[i] = std::make_unique<WorkGuard>(boost::asio::make_work_guard(_ioServices[i]));
	}

	for (std::size_t i = 0; i < _ioServices.size(); i++)
	{
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}

AsioIOServicePool::~AsioIOServicePool()
{
	std::cout << "AsioIOServicePool destruct\n";
	_work_guards.clear();

	for (auto& ios : _ioServices)
	{
		ios.stop();
	}

	for (std::thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
}

AsioIOServicePool::IOService& AsioIOServicePool::GetIOService()
{
	auto& service = _ioServices[_nextIOServiceIndex++];
	if (_nextIOServiceIndex >= _ioServices.size())
	{
		_nextIOServiceIndex = 0;
	}

	return service;
}

void AsioIOServicePool::Stop()
{
	for (auto& work : _work_guards)
	{
		work.reset();
	}

	for (auto& t : _threads)
	{
		t.join();
	}
}