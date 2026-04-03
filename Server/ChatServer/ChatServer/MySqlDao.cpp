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

    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE uid=?"));
        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> p_info = std::make_shared<UserInfo>();

        while (res->next()){
            p_info->email = res->getString("email");
            p_info->name = res->getString("name");
            p_info->uid = res->getInt("uid");
            p_info->pwd = res->getString("pwd");
            break;
        }

        pool_->returnConnection(std::move(con));
        return p_info;
    }
    catch(sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}