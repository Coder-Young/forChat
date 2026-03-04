#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include <memory>
#include <iostream>
#include <mutex>
#include <thread>
#include <QByteArray>
#include <QNetworkReply>
#include <QMap>
#include <QJsonObject>
#include <QDir>
#include <QApplication>
#include <QFile>
#include <QSettings>
#include "QStyle"

/**
 * @brief repolish 用来刷新qss
 */
extern std::function<void(QWidget*)> repolish;

enum ReqID{
    ID_GET_VERIFY_CODE  =    1001,
    ID_REG_USER         =    1002,
};

enum Modules{
    REGISTERMOD =   0,
};

enum ErrCode{
    SUCCESS     =   0,
    ERR_JSON    =   1,
    ERR_NETWORK =   2,
};

extern QString gate_url_prefix;

#endif // GLOBAL_H
