#include "RedisMgr.h"
#include "RedisConnectionGuard.h"

RedisMgr::RedisMgr()
{

}

RedisMgr::~RedisMgr()
{
    Close();
}

bool RedisMgr::Connect(const std::string &host, int port, const std::string& pwd)
{
    RedisConnectionGuard guard(conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto reply = (redisReply *)redisCommand(connect, "AUTH %s", pwd.c_str());
    if (reply->type == REDIS_REPLY_ERROR)
    {
        myspdlog::error("认证失败");
        return false;
    }

    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& code, const int sec)
{
    RedisConnectionGuard guard(conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "SET %s %s EX %d", key.c_str(), code.c_str(), sec);
    if (NULL == reply)
    {
        myspdlog::error("redisCommand failed");
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    if (!ok)
    {
        std::cerr << "Redis SET failed!" << std::endl;
        std::cerr << "   Reply type: " << reply->type << std::endl;
        if (reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "   Error message: " << _reply->str << std::endl;
        } else {
            std::cerr << "   Unexpected reply type, str: " 
                  << (reply->str ? reply->str : "(null)") << std::endl;
        }
        return false;
    }

    return true;
}

bool RedisMgr::ExistsKey(const std::string &key)
{
    RedisConnectionGuard guard(conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
    if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER || _reply->integer == 0) {
        return false;
    }

    return true;
}


void RedisMgr::Close()
{
    conn_pool_->Close();
}