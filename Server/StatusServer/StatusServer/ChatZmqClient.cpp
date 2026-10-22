#include "ChatZmqClient.h"
#include "CfgManager.h"
#include "ZmqConnectionPoolManager.h"

ChatZmqClient::ChatZmqClient()
{
    CfgManager cfg;
    auto server_list = cfg["ChatServers"]["ServerName"];

    std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

    while (std::getline(ss, word, ',')){
        words.push_back(word);
    }

    for (auto &server : words){
        std::string server_name = cfg[server.c_str()]["Name"];
        if (server_name.empty()){
            continue;
        }
        
        _pool[server_name] = ZmqConnectionPoolManager::GetInstance()->getConnectionPool(server_name);
    }
}