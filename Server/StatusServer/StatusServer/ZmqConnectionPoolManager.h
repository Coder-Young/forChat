#include "const.h"
#include "Singleton.h"
#include "CfgManager.h"
#include "ZmqConnectionPool.h"

class ZmqConnectionPoolManager : public Singleton<ZmqConnectionPoolManager>
{
public:
    std::shared_ptr<ZmqConnectionPool> getConnectionPool(std::string poolName){
        std::lock_guard<std::mutex> lk(_mutex);
        if (m_connectionPool.find(poolName) != m_connectionPool.end()){
            return m_connectionPool[poolName];
        }

        CfgManager cfg;
        std::string host = cfg[poolName]["Host"];
        std::string port = cfg[poolName]["Port"];
        if (host.empty() || port.empty())
            throw std::invalid_argument("Invalid host port");
        std::string endpoint = host + ":" + port;

        std::shared_ptr<ZmqConnectionPool> pool = std::make_shared<ZmqConnectionPool>(endpoint);
        m_connectionPool.insert(std::make_pair(poolName, pool));
        return pool;
    }

private:
    std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<ZmqConnectionPool>> m_connectionPool;
};