#pragma once
#include "const.h"
#include "Singleton.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() = default;

	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	void RegisterGetFunc(std::string url, HttpHandler handler) { _get_handlers.insert(std::make_pair(url, handler)); }
	void RegisterPostFunc(std::string url, HttpHandler handler) { _post_handlers.insert(std::make_pair(url, handler)); }

private:
	LogicSystem();

private:
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};

