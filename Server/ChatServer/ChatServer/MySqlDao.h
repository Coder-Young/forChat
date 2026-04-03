#pragma once
#include "const.h"
#include "MySqlPool.h"

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();

    std::shared_ptr<UserInfo> GetUser(int uid);

private:
    std::unique_ptr<MySqlPool> pool_;
};