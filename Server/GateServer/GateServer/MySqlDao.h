#pragma once
#include "const.h"
#include "MySqlPool.h"

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();
    int RegUser(const std::string &name, const std::string &email, const std::string& pwd);

private:
    std::unique_ptr<MySqlPool> pool_;
};