#include "httpmanager.h"

HttpManager::HttpManager()
{
    connect(this, &HttpManager::sig_http_finish, this, &HttpManager::slot_http_finished);
}

HttpManager::~HttpManager()
{

}

void HttpManager::PostHttpReq(QUrl url, QJsonObject json, ReqID req_id, Modules mod)
{
    QByteArray data = QJsonDocument(json).toJson();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

    auto self = shared_from_this();
    QNetworkReply* reply = _manager.post(request, data);
    QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod](){
        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << reply->errorString();
            emit self->sig_http_finish(req_id, "", ErrCode::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        QString res = reply->readAll();
        emit self->sig_http_finish(req_id, res, ErrCode::SUCCESS, mod);
        reply->deleteLater();
        return;
    });
}

void HttpManager::slot_http_finished(ReqID req_id, QString res, ErrCode err, Modules mod)
{
    switch (mod)
    {
    case Modules::REGISTERMOD:
    {
        emit sig_reg_mod_finish(req_id, res, err);
        break;
    }
    default:
        break;
    }
}
