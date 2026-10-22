#pragma once
#include "const.h"
#include "Singleton.h"
#include "MySqlDao.h"

class MySqlMgr : public Singleton<MySqlMgr>
{
    friend class Singleton<MySqlMgr>;
public:
    ~MySqlMgr();

    int RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const int& sex, const std::string& icon, const std::string& nick);
    bool checkEmail(const std::string& name, const std::string& email);
    bool updatePwd(const std::string& name, const std::string& newPwd);
    bool checkPwd(const std::string &name, const std::string &pwd, UserInfo& info);

private:
    MySqlMgr();
    MySqlDao _dao;
};