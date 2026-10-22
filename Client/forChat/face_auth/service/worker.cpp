#include "session.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QJsonDocument>
#include <cstdio>
#include <exception>
#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

namespace face {
static bool readPacket(QJsonObject *request)
{
    unsigned char header[4];
    if (fread(header,1,4,stdin) != 4) return false;
    quint32 length = (quint32(header[0]) << 24) | (quint32(header[1]) << 16) |
        (quint32(header[2]) << 8) | header[3];
    if (!length || length > 8192) return false;
    QByteArray data(int(length), Qt::Uninitialized);
    if (fread(data.data(),1,length,stdin) != length) return false;
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(data,&error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
    *request = document.object();
    return true;
}
static bool sendPacket(QJsonObject result, QImage &preview)
{
    QByteArray imageData;
    if (!preview.isNull()) {
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        preview.save(&buffer,"JPEG",65);
        result["preview"] = QString::fromLatin1(imageData.toBase64());
        preview.fill(0);
    }
    QByteArray data = QJsonDocument(result).toJson(QJsonDocument::Compact);
    wipe(imageData);
    const quint32 length = quint32(data.size());
    unsigned char header[4] = {uchar(length >> 24),uchar(length >> 16),uchar(length >> 8),uchar(length)};
    bool written = fwrite(header,1,4,stdout) == 4 &&
        fwrite(data.constData(),1,size_t(data.size()),stdout) == size_t(data.size()) && fflush(stdout) == 0;
    wipe(data);
    return written;
}
int runWorker()
{
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    PlatformSecureStorageAdapter storage;
    FaceAuthSession session(FaceAuthConfig::load(),storage);
    QJsonObject request;
    while (readPacket(&request)) {
        QJsonObject response;
        QImage preview;
        try {
            const QString operation = request.value("op").toString();
            if (operation == "start") response = session.start(request);
            else if (operation == "next") response = session.next(&preview);
            else if (operation == "cancel") {
                session.cancel();
                response = QJsonObject{{"done",true},{"success",false},{"message",QStringLiteral("已取消")}};
            } else throw Failure(FaceAuthError::Internal,QStringLiteral("无效操作"));
        } catch (const Failure &failure) {
            response = QJsonObject{{"done",true},{"success",false},{"error",int(failure.code)},{"message",failure.message}};
        } catch (const std::exception &) {
            response = QJsonObject{{"done",true},{"success",false},{"message",QStringLiteral("摄像头或模型处理失败，请使用密码登录")}};
        } catch (...) {
            response = QJsonObject{{"done",true},{"success",false},{"message",QStringLiteral("人脸任务异常，已安全退出")}};
        }
        if (response.value("done").toBool()) {
            session.cancel();
            preview.fill(0);
            preview = QImage();
        }
        if (!sendPacket(response,preview) || response.value("done").toBool()) break;
    }
    session.cancel();
    return 0;
}
}
