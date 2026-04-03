#pragma once
#include "const.h"
#include "Singleton.h"

class StatusZmqClient : public Singleton<StatusZmqClient>
{
    friend Singleton<StatusZmqClient>;
public:
    StatusZmqClient();
    ~StatusZmqClient();

    bool VerifyToken(int uid, std::string token);
};