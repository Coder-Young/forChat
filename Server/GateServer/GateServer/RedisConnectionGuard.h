#pragma once
#include <memory>
#include "RedisConPool.h"

class RedisConnectionGuard
{
public:
    RedisConnectionGuard(std::shared_ptr<RedisConPool> _pool) : 
        pool(_pool), connect(pool->getConnection()) {}
    
    ~RedisConnectionGuard(){
        pool->returnConnection(connect);
    }

    redisContext* get(){
        return connect;
    }

    RedisConnectionGuard(const RedisConnectionGuard&) = delete;
    RedisConnectionGuard& operator=(const RedisConnectionGuard&) = delete;

private:
    std::shared_ptr<RedisConPool> pool;
    redisContext* connect;
};