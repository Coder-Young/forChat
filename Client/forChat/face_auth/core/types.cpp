#include "types.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QSettings>
#include <cmath>

namespace face {
FaceAuthConfig FaceAuthConfig::load()
{
    QSettings settings(QCoreApplication::applicationDirPath() + "/face-auth.ini", QSettings::IniFormat);
    settings.beginGroup("FaceAuth");
    FaceAuthConfig config;
    config.enabled = settings.value("enabled", false).toBool();
    config.liveness = settings.value("liveness", false).toBool();
    config.debugMetrics = settings.value("debugMetrics", false).toBool();
    config.matchThreshold = settings.value("matchThreshold", 0.55).toDouble();
    config.detectionThreshold = settings.value("detectionThreshold", 0.9).toDouble();
    config.minSharpness = settings.value("minSharpness", 90).toDouble();
    config.minBrightness = settings.value("minBrightness", 45).toDouble();
    config.maxBrightness = settings.value("maxBrightness", 210).toDouble();
    config.maxRoll = settings.value("maxRoll", 15).toDouble();
    config.minFacePixels = settings.value("minFacePixels", 100).toInt();
    config.maxRetries = settings.value("maxRetries", 3).toInt();
    config.timeoutMs = settings.value("timeoutMs", 30000).toInt();
    config.cameraIndex = settings.value("cameraIndex", 0).toInt();
    config.modelDirectory = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(settings.value("modelDirectory", "models/face").toString());
    config.detectorFile = settings.value("detectorFile", "face_detection_yunet_2022mar.onnx").toString();
    config.embedderFile = settings.value("embedderFile", "face_recognition_sface_2021dec.onnx").toString();
    config.detectorSha256 = settings.value("detectorSha256").toString();
    config.embedderSha256 = settings.value("embedderSha256").toString();
    config.modelVersion = settings.value("modelVersion", "2021dec").toString();
    config.storagePolicy = settings.value("storagePolicy", "platform").toString();
    return config;
}
void FaceAuthConfig::validate() const
{
    if (!std::isfinite(matchThreshold) || matchThreshold < 0.3 || matchThreshold > 1 ||
        !std::isfinite(detectionThreshold) || detectionThreshold < 0.5 || detectionThreshold > 1 ||
        !std::isfinite(minSharpness) || minSharpness <= 0 ||
        !std::isfinite(minBrightness) || !std::isfinite(maxBrightness) ||
        minBrightness < 0 || maxBrightness > 255 || minBrightness >= maxBrightness ||
        !std::isfinite(maxRoll) || maxRoll <= 0 || maxRoll > 30 ||
        minFacePixels < 40 || minFacePixels > 480 || maxRetries < 1 || maxRetries > 10 ||
        timeoutMs < 1000 || timeoutMs > 120000 || cameraIndex < 0 || cameraIndex > 16 ||
        storagePolicy != "platform" || modelVersion.isEmpty())
        throw Failure(FaceAuthError::InvalidConfig, QStringLiteral("人脸配置无效，请检查 face-auth.ini"));
    if (liveness) throw Failure(FaceAuthError::LivenessUnavailable, QStringLiteral("未安装可靠活体检测器，已拒绝启动"));
}
void wipe(QByteArray &value)
{
    volatile char *bytes = value.data();
    for (int index = 0; index < value.size(); ++index) bytes[index] = 0;
    value.clear();
}
void wipe(QVector<float> &value)
{
    volatile float *values = value.data();
    for (int index = 0; index < value.size(); ++index) values[index] = 0;
    value.clear();
}
double cosine(const QVector<float> &left, const QVector<float> &right)
{
    if (left.size() != 128 || right.size() != 128) return -1;
    double dot = 0, leftNorm = 0, rightNorm = 0;
    for (int index = 0; index < left.size(); ++index) {
        if (!std::isfinite(left[index]) || !std::isfinite(right[index])) return -1;
        dot += double(left[index]) * right[index];
        leftNorm += double(left[index]) * left[index];
        rightNorm += double(right[index]) * right[index];
    }
    if (leftNorm < 1e-12 || rightNorm < 1e-12) return -1;
    return dot / std::sqrt(leftNorm * rightNorm);
}
QString storageKey(const QString &origin, const QString &account)
{
    return QCryptographicHash::hash(origin.toUtf8() + '\0' + account.toUtf8(), QCryptographicHash::Sha256).toHex();
}
}
