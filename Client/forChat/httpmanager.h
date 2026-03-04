#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H
#include "Singleton.h"
#include "global.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

class HttpManager : public QObject, public Singleton<HttpManager>,
                        public std::enable_shared_from_this<HttpManager>
{
    Q_OBJECT
public:
    ~HttpManager();

private:
    friend class Singleton<HttpManager>;
    HttpManager();

public:
    void PostHttpReq(QUrl url, QJsonObject json, ReqID req_id, Modules mod);

private:
    QNetworkAccessManager _manager;

private slots:
    void slot_http_finished(ReqID req_id, QString res, ErrCode err, Modules mod);

signals:
    void sig_http_finish(ReqID req_id, QString res, ErrCode err, Modules mod);
    void sig_reg_mod_finish(ReqID req_id, QString res, ErrCode err);
};

#endif // HTTPMANAGER_H
