#pragma once
#include "const.h"
#include "ConfigManager.h"
#include "Singleton.h"
#include "ZmqConnectionPool.h"

class ZmqPoolManager : public Singleton<ZmqPoolManager>
{
public:
    std::shared_ptr<ZmqConnectionPool> getPool(std::string name){
        auto it = pools_.find(name);
        if (it != pools_.end())
        {
            return it->second;
        }

        CfgManager cfg;
        std::string endpoint = "";
        if (name == "StatusServer")
        {
            endpoint = cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"];
        }
        else
        {
            throw std::invalid_argument("Unknown pool name");
        }

        auto pool = std::make_shared<ZmqConnectionPool>(endpoint);
        pools_[name] = pool;
        return pool;
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ZmqConnectionPool>> pools_;
};