#pragma once
#include "const.h"
#include "MySqlPool.h"

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();
    int RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const int& sex, const std::string& icon, const std::string& nick);
    bool CheckEmail(const std::string &name, const std::string &email);
    bool CheckPwd(const std::string &name, const std::string &pwd, UserInfo& info);
    bool UpdatePwd(const std::string &name, const std::string &newPwd);

private:
    std::unique_ptr<MySqlPool> pool_;
};