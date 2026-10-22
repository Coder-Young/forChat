#include "CServer.h"
#include "CSession.h"
#include "const.h"
#include "UserMgr.h"
#include "RedisMgr.h"
#include "ConfigManager.h"

CServer::CServer(boost::asio::io_context& ioc, short port)
    : _ioc(ioc), _acceptor(_ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , _timer(_ioc, std::chrono::seconds(60))
{

}

CServer::~CServer()
{
    _sessions.clear();
    _ioc.stop();
}

void CServer::start_accept()
{
    auto self = shared_from_this();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(_ioc, this);
    _acceptor.async_accept(new_session->getSocket(), [this, new_session, self](boost::system::error_code error){
        if (!error)
        {
            new_session->Start();
            std::lock_guard<std::mutex> lock(_mutex);
            _sessions.insert(make_pair(new_session->GetSessionId(), new_session));
        }
        else{
            std::cout << "session accept failed, error is " << error << std::endl;
        }
        
        start_accept();
    });
}

void CServer::ClearSession(std::string session_id) {
	
	std::lock_guard<std::mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		auto uid = _sessions[session_id]->GetUserId();
		UserMgr::GetInstance()->RmvUserSession(uid, session_id);
	}

	_sessions.erase(session_id);
}

bool CServer::CheckValid(std::string uuid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(uuid);
	if (it != _sessions.end()) {
		return true;
	}
	return false;
}

void CServer::on_timer(const boost::system::error_code& e)
{
    std::vector<std::shared_ptr<CSession>> _expired_sessions;
    int session_count = 0;
    //加锁遍历session
    {
        std::lock_guard<std::mutex> lock(_mutex);
        time_t now = std::time(nullptr);
        for (auto iter = _sessions.begin(); iter != _sessions.end(); iter++) {
            auto b_expired = iter->second->IsHeartbeatExpired(now);
            if (b_expired) {
                //这里也会触发async_read的错误处理
                iter->second->Close();
                //收集过期信息
                _expired_sessions.push_back(iter->second);
                continue;
            }
            session_count++;
        }
    }
    //设置session数量
    CfgManager cfg;
    auto self_name = cfg["SelfServer"]["Name"];
    auto count_str = std::to_string(session_count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

    //处理过期session, 单独提出，防止死锁
    for (auto &session : _expired_sessions) {
        session->DealExceptionSession();
    }

    //再次设置，下一个60s检测
    _timer.expires_after(std::chrono::seconds(60));
    _timer.async_wait([this](boost::system::error_code ec) {
        on_timer(ec);
    });
}

void CServer::StartTimer()
{
	auto self(shared_from_this());
	_timer.async_wait([self](boost::system::error_code ec) {
		self->on_timer(ec);
	});
}

void CServer::StopTimer()
{
	_timer.cancel();
}