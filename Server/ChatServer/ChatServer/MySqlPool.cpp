#include "MySqlPool.h"

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pwd, const std::string& schema, int poolSize/* = 3*/)
    : m_url(url), m_user(user), m_pwd(pwd), m_schema(schema), pool_size(poolSize)
{
    try
    {
        for (int i = 0; i < pool_size; i++)
        {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect(m_url, m_user, m_pwd));
            con->setSchema(m_schema);
            pool_.push(std::move(con));
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "mysql pool init failed" << std::endl;
    }
    
}

MySqlPool::~MySqlPool()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()){
        pool_.pop();
    }
}

std::unique_ptr<sql::Connection> MySqlPool::getConnection()
{
    std::unique_lock<std::mutex> unique_lk(mutex_);
    cond_.wait(unique_lk, [this](){
        if (!pool_.empty() || b_stop)
        {
            return true;
        }

        return false;
    });

    if (b_stop)
    {
        return nullptr;
    }

    std::unique_ptr<sql::Connection> con(std::move(pool_.front()));
    pool_.pop();
    return con;
}

void MySqlPool::returnConnection(std::unique_ptr<sql::Connection> con)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop)
        return;

    pool_.push(std::move(con));
    cond_.notify_one();
}

void MySqlPool::Close()
{
    b_stop = true;
    cond_.notify_all();
}

void MySqlPool::checkConnection()
{
    
}