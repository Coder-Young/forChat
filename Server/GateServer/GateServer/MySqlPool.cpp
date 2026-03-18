#include "MySqlPool.h"

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
    : m_url(url), m_user(user), m_pwd(pass), m_schema(schema), pool_size(poolSize), b_stop(false)
{
    try{
        for (int i = 0; i < pool_size; i++)
        {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect(m_url, m_user, m_pwd));
            con->setSchema(m_schema);
            pool_.push(std::move(con));
        }

        // _check_thread = std::thread([this](){
        //     checkConnection();
        // });
        // _check_thread.detach();
    }
    catch(std::exception &e){
        std::cout << "mysql pool init failed" << std::endl;
    }
}

MySqlPool::~MySqlPool()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty())
    {
        pool_.pop();
    }
}

void MySqlPool::checkConnection()
{

}

std::unique_ptr<sql::Connection> MySqlPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this](){
        if (b_stop)
            return true;

        return !pool_.empty();
    });

    if (b_stop){
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