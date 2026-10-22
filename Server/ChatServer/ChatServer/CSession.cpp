#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "RedisMgr.h"
#include "boost/uuid/uuid.hpp"
#include <boost/uuid/uuid_io.hpp>
#include "boost/uuid/random_generator.hpp"

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
    : _socket(ioc), _server(server), b_send_pending(false)
{
    boost::uuids::uuid auuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(auuid);
    _msg_head = std::make_shared<MsgNode>(MAX_MSG_SIZE);
}

void CSession::Start()
{
    AsyncReadHead(HEAD_LEN);
}

void CSession::Close()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _socket.close();
}

void CSession::AsyncReadHead(int total_len)
{
    auto self = shared_from_this();
    HandleReadFull(total_len, [self, this, total_len](boost::system::error_code& ec, size_t bytes_transfered){
        if (ec){
            std::cout << "[AsyncReadHead]read error: " << ec.message() << std::endl;
            Close();
            DealExceptionSession();
            return;
        }

        if (bytes_transfered < total_len){
            std::cout << "[AsyncReadHead]read not enough" << std::endl;
            Close();
            _server->ClearSession(_session_id);
            return;
        }

        // 分析头部
        _msg_head->Clear();
        memcpy(_msg_head->data, _data, bytes_transfered);

        short msg_id = 0;
        memcpy(&msg_id, _msg_head->data, HEAD_ID_LEN);
        msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);

        short msg_len = 0;
        memcpy(&msg_len, _msg_head->data + HEAD_ID_LEN, HEAD_DATA_LEN);
        msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

        if (bytes_transfered >= msg_len + HEAD_LEN){
            _msg_node = std::make_shared<RecvNode>(msg_id, msg_len);
            memcpy(_msg_node->data, _data + HEAD_LEN, msg_len);
            _msg_node->cur_len = msg_len;
            _msg_node->data[_msg_node->cur_len] = '\0';

            _logic_node = std::make_shared<LogicNode>(shared_from_this(), _msg_node);
            LogicSystem::GetInstance()->PostToMsgQueue(_logic_node);

            if (bytes_transfered > msg_len + HEAD_LEN){
                // 处理半包问题
            }
            else{
                AsyncReadHead(HEAD_LEN);
            }
        }
        else{
            int remaining_bytes = msg_len + HEAD_LEN - bytes_transfered;
            if (bytes_transfered > HEAD_LEN){
                _msg_node = std::make_shared<RecvNode>(msg_id, msg_len);
                memcpy(_msg_node->data, _data + HEAD_LEN, bytes_transfered - HEAD_LEN);
                _msg_node->cur_len += bytes_transfered - HEAD_LEN;
            }
            AsyncReadBody(remaining_bytes);
        }
    });
}

void CSession::AsyncReadBody(int body_len)
{
    auto self = shared_from_this();
    HandleReadFull(body_len, [self, this, body_len](boost::system::error_code& ec, size_t bytes_transfered){
        if (ec){
            std::cout << "[AsyncReadBody]read error: " << ec.message() << std::endl;
            Close();
            DealExceptionSession();
            return;
        }

        if (bytes_transfered < body_len)
        {
            std::cout << "[AsyncReadBody]read not enough" << std::endl;
            Close();
            _server->ClearSession(_session_id);
            return;
        }

        if (!_server->CheckValid(_session_id)){
            Close();
            return;
        }

        int need_len = _msg_node->max_len - _msg_node->cur_len;
        if (bytes_transfered >= need_len){
            memcpy(_msg_node->data + _msg_node->cur_len, _data, need_len);
            _msg_node->cur_len += need_len;
            _msg_node->data[_msg_node->cur_len] = '\0';
            _logic_node = std::make_shared<LogicNode>(shared_from_this(), _msg_node);
            
            UpdateHeartbeat();
            LogicSystem::GetInstance()->PostToMsgQueue(_logic_node);

            bytes_transfered -= need_len;
            if (bytes_transfered > 0){
                // 处理半包问题
            }
            else{
                AsyncReadHead(HEAD_LEN);
            }
        }
        else{
            AsyncReadBody(need_len - bytes_transfered);
        }
    });
}

void CSession::HandleReadFull(int total_len, handlerFunc handler)
{
    ::memset(_data, 0, max_length);
    HandleReadLen(0, total_len, handler);
}

void CSession::HandleReadLen(int cur_len, int total_len, handlerFunc handler)
{
    _socket.async_read_some(boost::asio::buffer(_data + cur_len, max_length - cur_len), 
        [this, handler, cur_len, total_len](boost::system::error_code err, size_t bytes_transfered) mutable {
            if (err){
                handler(err, bytes_transfered);
                return;
            }

            if (cur_len + bytes_transfered >= total_len)
            {
                handler(err, bytes_transfered);
                return;
            }

            cur_len += bytes_transfered;
            HandleReadLen(cur_len, total_len, handler);
        });
}

void CSession::Send(char* msg,  short max_length, short msgid)
{
    Send(std::string(msg, max_length), msgid);
}

void CSession::Send(std::string msg, short msgid)
{
    b_send_pending = false;
    std::lock_guard<std::mutex> lock(_send_lock);
    if (_send_queue.size() >= MAX_SENDQUE_SIZE){
        return;
    }

    if (_send_queue.size() > 0){
        b_send_pending = true;
    }

    _send_queue.push(std::make_shared<SendNode>(msg.c_str(), msgid, msg.size()));
    if (b_send_pending){
        return;
    }

    auto& msgnode = _send_queue.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->data, msgnode->max_len), std::bind(&CSession::HandleWrite, this, std::placeholders::_1, std::placeholders::_2, shared_from_this()));
}

void CSession::HandleWrite(boost::system::error_code err, std::size_t bytes_writed, std::shared_ptr<CSession> shared_self)
{
    try{
        if (!err){
            std::lock_guard<std::mutex> lock(_send_lock);
            _send_queue.pop();
            if (!_send_queue.empty()){
                auto& nextmsg = _send_queue.front();
                boost::asio::async_write(_socket, boost::asio::buffer(nextmsg->data, nextmsg->max_len), 
                    std::bind(&CSession::HandleWrite, this, std::placeholders::_1, std::placeholders::_2, shared_self));
            }
        }
        else{
            std::cout << "[CSession]Send fail " << err.message() << std::endl;
            Close();
        }
    }
    catch(std::exception& e){
        std::cerr << "Exception code : " << e.what() << std::endl;
    }
}

std::string& CSession::GetSessionId() {
	return _session_id;
}

void CSession::SetUserId(int uid)
{
	_user_uid = uid;
}

int CSession::GetUserId()
{
	return _user_uid;
}

void CSession::NotifyOffline(int uid) {

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;

	std::string return_str = rtvalue.toStyledString();
	Send(return_str, ID_NOTIFY_OFF_LINE_REQ);
	return;
}

bool CSession::IsHeartbeatExpired(std::time_t& now) {
	double diff_sec = std::difftime(now, _last_heartbeat);
	if (diff_sec > 20) {
		std::cout << "heartbeat expired, session id is  " << _session_id << std::endl;
		return true;
	}

	return false;
}

void CSession::UpdateHeartbeat()
{
	time_t now = std::time(nullptr);
	_last_heartbeat = now;
}

void CSession::DealExceptionSession()
{
	auto self = shared_from_this();
	auto uid_str = std::to_string(_user_uid);

	auto lock_key = LOCK_PREFIX + uid_str;
	auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	Defer defer([identifier, lock_key, self, this]() {
		_server->ClearSession(_session_id);
		RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
	});

	if (identifier.empty()) {
		return;
	}

	std::string redis_session_id = "";
	auto bsuccess = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id);
	if (!bsuccess) {
		return;
	}

	if (redis_session_id != _session_id) {
		return;
	}

	RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
	RedisMgr::GetInstance()->Del(USERIPPERFIX + uid_str);
}
