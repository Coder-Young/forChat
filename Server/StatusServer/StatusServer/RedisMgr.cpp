#include "RedisMgr.h"
#include "RedisConnectionGuard.h"
#include "CfgManager.h"

RedisMgr::RedisMgr()
{
    CfgManager mgr = CfgManager();
    auto host = mgr["Redis"]["Host"];
    auto port = mgr["Redis"]["Port"];
    auto pwd = mgr["Redis"]["Passwd"];
    conn_pool_.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr()
{
    conn_pool_->Close();
}

bool RedisMgr::Get(const std::string &key, std::string& value)
{
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
    if (_reply == NULL) {
        myspdlog::error("[RedisMgr] GET {} failed", key);
        freeReplyObject(_reply);
        return false;
    }

    if (_reply->type != REDIS_REPLY_STRING) {
         myspdlog::error("[RedisMgr] GET {} failed", key);
         freeReplyObject(_reply);
         return false;
    }

    value = _reply->str;
    freeReplyObject(_reply);

    return true;
}

bool RedisMgr::Set(const std::string &key, const std::string &value){
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    if (NULL == _reply)
    {
        myspdlog::error("[RedisMgr] SET {} {} failed", key, value);
        freeReplyObject(_reply);
        return false;
    }
    
    if (!(_reply->type == REDIS_REPLY_STATUS && (strcmp(_reply->str, "OK") == 0 || strcmp(_reply->str, "ok") == 0)))
    {
        myspdlog::error("[RedisMgr] SET {} {} failed", key, value);
        freeReplyObject(_reply);     
        return false;
    }
    
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::Auth(const std::string &password)
{
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());
    if (_reply->type == REDIS_REPLY_ERROR) {
        std::cout << "认证失败" << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    else {
        freeReplyObject(_reply);
        std::cout << "认证成功" << std::endl;
        return true;
    }
}

bool RedisMgr::LPush(const std::string &key, const std::string &value)
{
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == _reply)
    {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER || _reply->integer <= 0) {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::LPop(const std::string &key, std::string& value){
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());
    if (_reply == nullptr || _reply->type == REDIS_REPLY_NIL) {
        std::cout << "Execut command [ LPOP " << key<<  " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }

    value = _reply->str;
    std::cout << "Execut command [ LPOP " << key <<  " ] success ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == _reply)
    {
        std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    if (_reply->type != REDIS_REPLY_INTEGER || _reply->integer <= 0) {
        std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
    if (_reply == nullptr || _reply->type == REDIS_REPLY_NIL) {
        std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    value = _reply->str;
    std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::HSet(const std::string &key, const std::string &hkey, const std::string &value) {
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER ) {
        std::cout << "Execut command [ HSet " << key << "  " << hkey <<"  " << value << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;

    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
    if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

std::string RedisMgr::HGet(const std::string &key, const std::string &hkey)
{
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();

    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return "";
    }

    auto _reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
    if (_reply == nullptr || _reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(_reply);
        std::cout << "Execut command [ HGet " << key << " "<< hkey <<"  ] failure ! " << std::endl;
        return "";
    }
    std::string value = _reply->str;
    freeReplyObject(_reply);
    return value;
}

bool RedisMgr::Del(const std::string &key)
{
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
    if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ Del " << key <<  " ] failure ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
     freeReplyObject(_reply);
     return true;
}

bool RedisMgr::ExistsKey(const std::string &key)
{
    RedisConnectionGuard guard(*conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    auto _reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
    if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER || _reply->integer == 0) {
        std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
        freeReplyObject(_reply);
        return false;
    }
    std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
    freeReplyObject(_reply);
    return true;
}

// 分布式锁相关
std::string RedisMgr::acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout)
{
    RedisConnectionGuard guard(conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return "";
    }

    return DistLock::Inst().acquireLock(connect, lockName, lockTimeout, acquireTimeout);
}

bool RedisMgr::releaseLock(const std::string& lockName, const std::string& identifier)
{
    if (identifier.empty()) {
        return true;
    }

    RedisConnectionGuard guard(conn_pool_);
    auto connect = guard.get();
    if (connect == nullptr)
    {
        return false;
    }

    return DistLock::Inst().releaseLock(connect, lockName, identifier);
}


void RedisMgr::Close()
{
    conn_pool_->Close();
}