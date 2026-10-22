#include "MySqlDao.h"
#include "CfgManager.h"
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

int MySqlDao::RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const int& sex, const std::string& icon, const std::string& nick)
{
    auto con = pool_->getConnection();
    try{
        if (con == nullptr){
            return -1;
        }

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("CALL reg_user(?,?,?,?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);
        stmt->setInt(4, sex);
        stmt->setString(5, icon);
        stmt->setString(6, nick);

        stmt->execute();
        std::unique_ptr<sql::Statement> stmtResult(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next())
        {
          int result = res->getInt("result");
          std::cout << "Result: " << result << std::endl;
          pool_->returnConnection(std::move(con));
          return result; 
        }

        pool_->returnConnection(std::move(con));
        return 0;
    }
    catch(sql::SQLException& e){
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool MySqlDao::CheckEmail(const std::string &name, const std::string &email)
{
    auto con = pool_->getConnection();
    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT email FROM user WHERE name=?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            if (email != res->getString("email"))
            {
                pool_->returnConnection(std::move(con));
                return false;
            }

            pool_->returnConnection(std::move(con));
            return true;
        }

        pool_->returnConnection(std::move(con));
        return true;
    }
    catch(sql::SQLException &e){
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MySqlDao::CheckPwd(const std::string &name, const std::string &pwd, UserInfo& info)
{
    auto con = pool_->getConnection();
    if (con == nullptr){
        return false;
    }

    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE name=?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string inner_pwd = "";
        while (res->next()){
            inner_pwd = res->getString("pwd");
            break;
        }

        std::cout << "MySqlDao end while" << std::endl;
        if (pwd != inner_pwd)
        {
            pool_->returnConnection(std::move(con));
            return false;
        }

        info.name = name;
        info.email = res->getString("email");
        info.uid = res->getInt("uid");
        info.pwd = inner_pwd;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch(sql::SQLException &e){
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MySqlDao::UpdatePwd(const std::string &name, const std::string &newPwd)
{
    auto con = pool_->getConnection();
    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("UPDATE user SET pwd=? WHERE name=?"));
        pstmt->setString(1, newPwd);
        pstmt->setString(2, name);

        int updateCount = pstmt->executeUpdate();
        std::cout << "Updated rows: " << updateCount << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch(sql::SQLException &e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}