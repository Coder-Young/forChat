#include "const.h"
#include "Singleton.h"
#include "MySqlDao.h"

class MySqlMgr : public Singleton<MySqlMgr>
{
    friend class Singleton<MySqlMgr>;
public:
    ~MySqlMgr();

    std::shared_ptr<UserInfo> GetUser(int uid);
    
private:
    MySqlMgr();
    MySqlDao _dao;
};