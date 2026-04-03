#pragma once
#include "const.h"
#include "RedisConPool.h"

class RedisConnectionGuard
{
public:
    explicit RedisConnectionGuard(RedisConPool& pool)
        : pool_(pool), conn_(pool.getConnection()) {}

    ~RedisConnectionGuard(){
        if (conn_ != nullptr){
            pool_.returnConnection(conn_);
        }
    }

    RedisConnectionGuard(const RedisConnectionGuard&) = delete;
    RedisConnectionGuard& operator=(const RedisConnectionGuard&) = delete;

    RedisConnectionGuard(RedisConnectionGuard&& rhs) noexcept
        : pool_(rhs.pool_), conn_(rhs.conn_)
    {
        rhs.conn_ = nullptr;
    }

    explicit operator bool() const{
        return conn_ != nullptr;
    }

    redisContext* get() const {
        return conn_;
    }

private:
    redisContext* conn_;
    RedisConPool& pool_;
};