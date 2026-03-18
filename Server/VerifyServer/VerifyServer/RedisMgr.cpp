#include "RedisMgr.h"

RedisMgr::RedisMgr()
{

}

RedisMgr::~RedisMgr()
{
    Close();
}

bool RedisMgr::Connect(const std::string &host, int port, const std::string& pwd)
{
    this->_connect = redisConnect(host.c_str(), port);
    if (this->_connect != NULL && this->_connect->err)
    {
        std::cout << "redis connect error " << this->_connect->errstr << std::endl;
        return false;
    }

    auto reply = (redisReply *)redisCommand(_connect, "AUTH %s", pwd.c_str());
    if (reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "认证失败" << std::endl;
        freeReplyObject(reply);
        return false;
    }

    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& code, const int sec)
{
    _reply = (redisReply*)redisCommand(this->_connect, "SET %s %s EX %d", key.c_str(), code.c_str(), sec);
    if (NULL == _reply)
    {
        std::cout << "redisCommand failed" << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    bool ok = (_reply->type == REDIS_REPLY_STATUS && strcmp(_reply->str, "OK") == 0);
    
    if (!ok)
    {
        std::cerr << "Redis SET failed!" << std::endl;
        std::cerr << "   Reply type: " << _reply->type << std::endl;
        if (_reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "   Error message: " << _reply->str << std::endl;
        } else {
            std::cerr << "   Unexpected reply type, str: " 
                  << (_reply->str ? _reply->str : "(null)") << std::endl;
        }
        freeReplyObject(_reply);
        return false;
    }

    std::cout << "set " << key << " success" << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::ExistsKey(const std::string &key)
{
    auto _reply = (redisReply*)redisCommand(this->_connect, "exists %s", key.c_str());
    if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER || _reply->integer == 0) {
        freeReplyObject(_reply);
        return false;
    }

    std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}


void RedisMgr::Close()
{
    redisFree(_connect);
}