#include "storage.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>

namespace face {
QByteArray serializeTemplate(const FaceTemplate &value)
{
    QJsonArray embedding;
    for (float component : value.embedding) embedding.append(double(component));
    return QJsonDocument(QJsonObject{{"userId",value.userId},{"account",value.account},{"origin",value.origin},
        {"modelId",value.modelId},{"modelVersion",value.modelVersion},{"embedding",embedding},
        {"createdAt",value.createdAt.toString(Qt::ISODateWithMs)},{"updatedAt",value.updatedAt.toString(Qt::ISODateWithMs)},
        {"templateVersion",value.templateVersion}}).toJson(QJsonDocument::Compact);
}
FaceTemplate parseTemplate(const QByteArray &data, const QString &origin, const QString &account,
                           const FaceAuthConfig &config)
{
    auto corrupt = []() { throw Failure(FaceAuthError::CorruptTemplate, QStringLiteral("人脸数据损坏、账号不符或模型版本已变化，请重新注册")); };
    if (data.size() > 16384) corrupt();
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(data,&error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) corrupt();
    QJsonObject object = document.object();
    FaceTemplate value;
    value.userId = object.value("userId").toString();
    value.account = object.value("account").toString();
    value.origin = object.value("origin").toString();
    value.modelId = object.value("modelId").toString();
    value.modelVersion = object.value("modelVersion").toString();
    value.templateVersion = object.value("templateVersion").toInt();
    value.createdAt = QDateTime::fromString(object.value("createdAt").toString(),Qt::ISODateWithMs);
    value.updatedAt = QDateTime::fromString(object.value("updatedAt").toString(),Qt::ISODateWithMs);
    if (value.userId.toInt() <= 0 || value.account != account || value.origin != origin ||
        value.modelId != config.modelId || value.modelVersion != config.modelVersion + ":" + config.embedderSha256 ||
        value.templateVersion != 1 || !value.createdAt.isValid() || !value.updatedAt.isValid()) corrupt();
    QJsonArray embedding = object.value("embedding").toArray();
    if (embedding.size() != 128) corrupt();
    double norm = 0;
    for (const QJsonValue &component : embedding) {
        if (!component.isDouble() || !std::isfinite(component.toDouble()) || std::abs(component.toDouble()) > 1.001) corrupt();
        value.embedding.append(float(component.toDouble()));
        norm += component.toDouble() * component.toDouble();
    }
    if (norm < 0.98 || norm > 1.02) { wipe(value.embedding); corrupt(); }
    return value;
}
}
