#pragma
#include "const.h"
#include "Singleton.h"
#include "CfgManager.h"
#include "ZmqConnectionPool.h"

class ZmqPoolManager : public Singleton<ZmqPoolManager>
{
public:
    std::shared_ptr<ZmqConnectPool> getPool(const std::string& name)
    {
        auto it = pools_.find(name);
        if (it != pools_.end())
        {
            return it->second;
        }

        CfgManager cfg;
        std::string endpoint = "";
        if (name == "VerifyServer")
        {
            endpoint = cfg["VerifyServer"]["Host"] + ":" + cfg["VerifyServer"]["Port"];
        }
        else if (name == "StatusServer")
        {
            endpoint = cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"];
        }
        else
        {
            throw std::invalid_argument("unknown pool name");
        }

        auto pool = std::make_shared<ZmqConnectPool>(endpoint);
        pools_[name] = pool;
        return pool;
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ZmqConnectPool>> pools_;
};