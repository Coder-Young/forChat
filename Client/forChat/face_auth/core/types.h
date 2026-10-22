#pragma once
#include <QDateTime>
#include <QImage>
#include <QJsonObject>
#include <QVector>
#include <stdexcept>

namespace face {
enum class FaceAuthError { None, Disabled, InvalidConfig, NoTemplate, CorruptTemplate,
    ModelMissing, ModelInvalid, CameraUnavailable, NoFace, MultipleFaces, TooSmall,
    Tilted, Blurry, Exposure, Mismatch, Cancelled, Timeout, StorageUnavailable,
    LivenessUnavailable, Internal };
struct FaceAuthResult {
    bool success = false;
    double score = 0;
    FaceAuthError errorCode = FaceAuthError::None;
    QString userMessage;
    bool retryable = false;
};
class Failure : public std::runtime_error {
public:
    FaceAuthError code;
    QString message;
    Failure(FaceAuthError value, const QString &text)
        : std::runtime_error("face operation failed"), code(value), message(text) {}
};
struct FaceTemplate {
    QString userId, account, origin, modelId, modelVersion;
    QVector<float> embedding;
    QDateTime createdAt, updatedAt;
    int templateVersion = 1;
};
struct FaceQualityResult {
    FaceAuthError error = FaceAuthError::None;
    QString message;
    double brightness = 0, sharpness = 0, roll = 0, yaw = 0;
    bool accepted() const { return error == FaceAuthError::None; }
};
struct FaceAuthConfig {
    bool enabled = false, liveness = false, debugMetrics = false;
    double matchThreshold = 0.55, detectionThreshold = 0.9;
    double minSharpness = 90, minBrightness = 45, maxBrightness = 210, maxRoll = 15;
    int minFacePixels = 100, maxRetries = 3, timeoutMs = 30000, cameraIndex = 0;
    QString modelDirectory, detectorFile, embedderFile, detectorSha256, embedderSha256;
    QString modelId = "opencv-sface", modelVersion = "2021dec";
    QString storagePolicy = "platform";
    static FaceAuthConfig load();
    void validate() const;
};
void wipe(QByteArray &value);
void wipe(QVector<float> &value);
double cosine(const QVector<float> &left, const QVector<float> &right);
QString storageKey(const QString &origin, const QString &account);
}
