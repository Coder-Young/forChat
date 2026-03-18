#pragma once
#include "const.h"
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>

class MySqlPool
{
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize);
    ~MySqlPool();

    std::unique_ptr<sql::Connection> getConnection();
    void returnConnection(std::unique_ptr<sql::Connection> con);
    void Close();

    void checkConnection();
private:
    std::string m_url;
    std::string m_user;
    std::string m_pwd;
    std::string m_schema;

    int pool_size;
    std::queue<std::unique_ptr<sql::Connection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop;

    std::thread _check_thread;
};