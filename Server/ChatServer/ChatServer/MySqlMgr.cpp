#include "MySqlMgr.h"

MySqlMgr::MySqlMgr()
{
}

MySqlMgr::~MySqlMgr()
{
}

std::shared_ptr<UserInfo> MySqlMgr::GetUser(int uid)
{
    return _dao.GetUser(uid);
}