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

std::shared_ptr<UserInfo> MySqlMgr::GetUser(std::string name)
{
    return _dao.GetUser(name);
}

bool MySqlMgr::AddFriendApply(const int& from, const int& to)
{
    return _dao.AddFriendApply(from, to);
}

bool MySqlMgr::AcceptFriendApply(const int& from, const int& to)
{
    return _dao.AcceptFriendApply(from, to);
}

bool MySqlMgr::AddFriend(const int& from, const int& to, std::string back_name)
{
    return _dao.AddFriend(from, to, back_name);
}

bool MySqlMgr::GetApplyList(int touid, 
	std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {

	return _dao.GetApplyList(touid, applyList, begin, limit);
}

bool MySqlMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info) {
	return _dao.GetFriendList(self_id, user_info);
}