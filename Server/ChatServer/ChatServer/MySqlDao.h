#pragma once
#include "const.h"
#include "MySqlPool.h"

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();

    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);

    bool AddFriendApply(const int& from, const int& to);
	bool AcceptFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string back_name);

    bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit );
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);

private:
    std::unique_ptr<MySqlPool> pool_;
};