#include "MySqlMgr.h"

MySqlMgr::MySqlMgr()
{
}

MySqlMgr::~MySqlMgr()
{
}

int MySqlMgr::RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const int& sex, const std::string& icon, const std::string& nick)
{
    return _dao.RegUser(name, email, pwd, sex, icon, nick);
}

bool MySqlMgr::checkEmail(const std::string& name, const std::string& email)
{
    return _dao.CheckEmail(name, email);
}

bool MySqlMgr::updatePwd(const std::string& name, const std::string& newPwd)
{
    return _dao.UpdatePwd(name, newPwd);
}

bool MySqlMgr::checkPwd(const std::string &name, const std::string &pwd, UserInfo& info)
{
    return _dao.CheckPwd(name, pwd, info);
}