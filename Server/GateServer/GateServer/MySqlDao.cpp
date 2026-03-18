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

int MySqlDao::RegUser(const std::string &name, const std::string &email, const std::string& pwd)
{
    auto con = pool_->getConnection();
    try{
        if (con == nullptr){
            return -1;
        }

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

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