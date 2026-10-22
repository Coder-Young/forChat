#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <functional>
#include <zmq.hpp>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <hiredis/hiredis.h>

#define USERTOKENPREFIX "USERTOKEN_"
#define LOGIN_COUNT "LOGINCOUNT_"
#define IPCOUNTPERFIX "IPCOUNT_"
#define USERIPPERFIX "USERIP_"
#define USERBASEINFO "USERBASEINFO_"
#define NAME_INFO  "nameinfo_"
#define USER_SESSION_PREFIX "usession_"
#define LOCK_PREFIX "lock_"

#define MAX_MSG_SIZE    4096
#define MAX_SENDQUE_SIZE 1000
#define HEAD_ID_LEN     2
#define HEAD_DATA_LEN   2
#define HEAD_LEN        (HEAD_ID_LEN+HEAD_DATA_LEN)
#define LOCK_TIME_OUT 10
#define ACQUIRE_TIME_OUT 5

enum MSG_ID{
    ID_GET_VARIFY_CODE = 1001, //获取验证码
    ID_REG_USER = 1002, //注册用户
    ID_RESET_PWD = 1003, //重置密码
    ID_LOGIN_USER = 1004, //用户登录
    ID_CHAT_LOGIN_REQ = 1005, //登陆聊天服务器
    ID_CHAT_LOGIN_RSP= 1006, //登陆聊天服务器回包
    ID_SEARCH_USER_REQ = 1007, //用户搜索请求
    ID_SEARCH_USER_RSP = 1008, //搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,  //添加好友申请
    ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
    ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ  = 1017,  //文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP  = 1018,  //文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
    ID_NOTIFY_OFF_LINE_REQ = 1021, //通知用户下线
    ID_HEART_BEAT_REQ = 1023,      //心跳请求
    ID_HEARTBEAT_RSP = 1024,       //心跳回复
};

enum ErrorCodes{
    Success =         0,
    UidInvalid =      1001,
    TokenInvalid =    1002,
    VerifyFailed =    1003,
};

struct UserInfo {
	UserInfo() :
        name(""), pwd(""),uid(0),email(""),nick(""),desc(""),sex(0), icon(""), back("")
    {}

	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string nick;
	std::string desc;
	int sex;
	std::string icon;
	std::string back;
};

struct ApplyInfo {
	ApplyInfo(int uid, std::string name, std::string desc,
		std::string icon, std::string nick, int sex, int status)
		:_uid(uid),_name(name),_desc(desc),
		_icon(icon),_nick(nick),_sex(sex),_status(status){}

	int _uid;
	std::string _name;
	std::string _desc;
	std::string _icon;
	std::string _nick;
	int _sex;
	int _status;
};

class Defer
{
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer()
    {
        func_();
    }

private:
    std::function<void()> func_;
};