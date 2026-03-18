#include "const.h"
#include "Singleton.h"

class RedisMgr: public Singleton<RedisMgr>, 
    public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
    
public:
    ~RedisMgr();

    bool Connect(const std::string &host, int port, const std::string& pwd);
    bool Set(const std::string& key, const std::string& value, const int sec);
    bool ExistsKey(const std::string &key);
    void Close();

private:
    RedisMgr();
    
    redisContext* _connect;
    redisReply* _reply;
};