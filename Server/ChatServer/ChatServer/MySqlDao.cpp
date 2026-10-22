#include "MySqlDao.h"
#include "ConfigManager.h"
#include <cppconn/prepared_statement.h>

MySqlDao::MySqlDao()
{
    CfgManager cfg;
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host+":"+port, user, pwd, schema, 5));
}

MySqlDao::~MySqlDao()
{

}

std::shared_ptr<UserInfo> MySqlDao::GetUser(int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr){
        return nullptr;
    }

    Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE uid=?"));
        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> p_info = nullptr;

        while (res->next()){
			p_info = std::make_shared<UserInfo>();
			p_info->pwd = res->getString("pwd");
			p_info->email = res->getString("email");
			p_info->name= res->getString("name");
			p_info->nick = res->getString("nick");
			p_info->desc = res->getString("desc");
			p_info->sex = res->getInt("sex");
			p_info->icon = res->getString("icon");
			p_info->uid = uid;
            break;
        }

        return p_info;
    }
    catch(sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MySqlDao::GetUser(std::string name)
{
    auto con = pool_->getConnection();
    if (con == nullptr){
        return nullptr;
    }

    Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE name=?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> p_info = nullptr;

        while (res->next()){
			p_info = std::make_shared<UserInfo>();
			p_info->pwd = res->getString("pwd");
			p_info->email = res->getString("email");
			p_info->name= name;
			p_info->nick = res->getString("nick");
			p_info->desc = res->getString("desc");
			p_info->sex = res->getInt("sex");
			p_info->icon = res->getString("icon");
			p_info->uid = res->getInt("uid");
            break;
        }

        return p_info;
    }
    catch(sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

bool MySqlDao::AddFriendApply(const int& from, const int& to)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
		pstmt->setInt(1, from);
		pstmt->setInt(2, to);

		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}

bool MySqlDao::AcceptFriendApply(const int& from, const int& to)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?"));
		
		pstmt->setInt(1, to);
		pstmt->setInt(2, from);
		
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}

		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}

bool MySqlDao::AddFriend(const int& from, const int& to, std::string back_name)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		con->setAutoCommit(false);

		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
			));
		
		pstmt->setInt(1, from);
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);
		
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt2(con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
		));

		pstmt2->setInt(1, to);
		pstmt2->setInt(2, from);
		pstmt2->setString(3, "");

		int rowAffected2 = pstmt2->executeUpdate();
		if (rowAffected2 < 0) {
			con->rollback();
			return false;
		}

		con->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}


	return true;
}

bool MySqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


		try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("select apply.from_uid, apply.status, user.name, "
				"user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? "));

		pstmt->setInt(1, touid);
		pstmt->setInt(2, begin);
		pstmt->setInt(3, limit);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {	
			auto name = res->getString("name");
			auto uid = res->getInt("from_uid");
			auto status = res->getInt("status");
			auto nick = res->getString("nick");
			auto sex = res->getInt("sex");
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
			applyList.push_back(apply_ptr);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MySqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info_list) {

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM friend WHERE self_id = ? "));

		pstmt->setInt(1, self_id);
	
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {		
			auto friend_id = res->getInt("friend_id");
			auto back = res->getString("back");
			auto user_info = GetUser(friend_id);
			if (user_info == nullptr) {
				continue;
			}

			user_info->back = user_info->name;
			user_info_list.push_back(user_info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}