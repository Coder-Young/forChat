#include "CSession.h"
#include "LogicSystem.h"

CSession::CSession(boost::asio::io_context& ioc)
    : _socket(ioc), b_pending(false)
{
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
            return;
        }

        if (bytes_transfered < total_len){
            std::cout << "[AsyncReadHead]read not enough" << std::endl;
            Close();
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

        _msg_node = std::make_shared<RecvNode>(msg_id, msg_len);
        _msg_node->Clear();
        memcpy(_msg_node->data, _msg_head->data + HEAD_LEN, msg_len);
        _msg_node->cur_len += bytes_transfered;
        _msg_node->data[_msg_node->max_len] = '\0';
        _logic_node = std::make_shared<LogicNode>(shared_from_this(), _msg_node);
        LogicSystem::GetInstance()->PostToMsgQueue(_logic_node);
        AsyncReadHead(HEAD_LEN);
        //AsyncReadBody(msg_len);
    });
}

void CSession::AsyncReadBody(int body_len)
{
    auto self = shared_from_this();
    HandleReadFull(body_len, [self, this, body_len](boost::system::error_code& ec, size_t bytes_transfered){
        if (ec){
            std::cout << "[AsyncReadBody]read error: " << ec.message() << std::endl;
            Close();
            return;
        }

        if (bytes_transfered < body_len)
        {
            std::cout << "[AsyncReadBody]read not enough" << std::endl;
            Close();
            return;
        }

        _msg_node->Clear();
        memcpy(_msg_node->data, _data, bytes_transfered);
        _msg_node->cur_len += bytes_transfered;
        _msg_node->data[_msg_node->max_len] = '\0';

        _logic_node = std::make_shared<LogicNode>(shared_from_this(), _msg_node);
        LogicSystem::GetInstance()->PostToMsgQueue(_logic_node);
        AsyncReadHead(HEAD_LEN);
    });
}

void CSession::HandleReadFull(int total_len, handlerFunc handler)
{
    ::memset(_data, 0, max_length);
    HandleReadLen(0, total_len, handler);
}

void CSession::HandleReadLen(int cur_len, int total_len, handlerFunc handler)
{
    _socket.async_read_some(boost::asio::buffer(_data, max_length), 
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
    b_pending = false;
    std::lock_guard<std::mutex> lock(_send_lock);
    if (_send_queue.size() >= MAX_SENDQUE_SIZE){
        return;
    }

    if (_send_queue.size() > 0){
        b_pending = true;
    }

    _send_queue.push(std::make_shared<SendNode>(msg.c_str(), msgid, msg.size()));
    if (b_pending){
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