#include "session.h"
#include <QJsonArray>
#include <cmath>

namespace face {
FaceTemplate FaceTemplateService::load(const QString &origin, const QString &account)
{
    QByteArray data = storage.read(storageKey(origin, account));
    if (data.isEmpty()) throw Failure(FaceAuthError::NoTemplate, QStringLiteral("该账号没有本地人脸模板，请使用密码登录"));
    try {
        FaceTemplate result = parseTemplate(data, origin, account, config);
        wipe(data);
        return result;
    } catch (...) { wipe(data); throw; }
}
void FaceTemplateService::save(const FaceTemplate &value)
{
    QByteArray data = serializeTemplate(value);
    try { storage.write(storageKey(value.origin, value.account), data); }
    catch (...) { wipe(data); throw; }
    wipe(data);
}
void FaceTemplateService::remove(const QString &origin, const QString &account)
{
    storage.remove(storageKey(origin, account));
}
FaceEnrollmentService::~FaceEnrollmentService() { clear(); }
void FaceEnrollmentService::clear()
{
    for (QVector<float> &sample : samples) wipe(sample);
    samples.clear();
}
QString FaceEnrollmentService::guidance() const
{
    if (samples.isEmpty()) return QStringLiteral("请正对摄像头");
    if (samples.size() == 1) return QStringLiteral("请轻微向一侧转头");
    return QStringLiteral("请轻微向另一侧转头");
}
bool FaceEnrollmentService::acceptsPose(double yaw) const
{
    if (samples.isEmpty()) return std::abs(yaw) < 0.10;
    if (samples.size() == 1) return yaw > 0.12 && yaw < 0.35;
    return yaw < -0.12 && yaw > -0.35;
}
bool FaceEnrollmentService::collect(QVector<float> embedding, double yaw, double threshold)
{
    bool accepted = samples.size() < 3 && acceptsPose(yaw) && cosine(embedding, embedding) > 0.99;
    if (!samples.isEmpty()) accepted = accepted && cosine(embedding, samples.first()) >= threshold;
    if (accepted) samples.append(embedding);
    else wipe(embedding);
    return accepted;
}
QVector<float> FaceEnrollmentService::finish() const
{
    if (samples.size() != 3) throw Failure(FaceAuthError::NoFace, QStringLiteral("尚未完成三个角度的采集"));
    QVector<float> mean(128,0);
    for (const QVector<float> &sample : samples)
        for (int index = 0; index < 128; ++index) mean[index] += sample[index];
    double norm = 0;
    for (float value : mean) norm += double(value) * value;
    if (!std::isfinite(norm) || norm < 1e-12)
        throw Failure(FaceAuthError::CorruptTemplate, QStringLiteral("特征提取无效，请重新采集"));
    for (float &value : mean) value /= float(std::sqrt(norm));
    return mean;
}
FaceAuthResult FaceLoginService::verify(const QVector<float> &probe, const FaceTemplate &saved,
                                       const FaceAuthConfig &config)
{
    FaceAuthResult result;
    if (failures >= config.maxRetries) {
        result.errorCode = FaceAuthError::Mismatch;
        result.userMessage = QStringLiteral("已达到最大重试次数，请使用密码登录");
        return result;
    }
    result.score = cosine(probe, saved.embedding);
    result.success = result.score >= config.matchThreshold;
    if (!result.success) {
        ++failures;
        result.errorCode = FaceAuthError::Mismatch;
        result.retryable = failures < config.maxRetries;
        result.userMessage = result.retryable ? QStringLiteral("未匹配，请调整姿势后重试") :
            QStringLiteral("已达到最大重试次数，请使用密码登录");
    } else result.userMessage = QStringLiteral("本地验证通过，请继续服务器密码认证");
    return result;
}
FaceAuthSession::FaceAuthSession(const FaceAuthConfig &settings, IBiometricStorage &backend)
    : config(settings), storage(backend) {}
FaceAuthSession::~FaceAuthSession() { cancel(); }
void FaceAuthSession::cancel()
{
    if (algorithm.camera) algorithm.camera->close();
    algorithm = Algorithm();
    enrollment.clear();
    wipe(saved.embedding);
    started = false;
}
QJsonObject FaceAuthSession::start(const QJsonObject &request)
{
    if (started) throw Failure(FaceAuthError::Internal, QStringLiteral("任务已启动"));
    mode = request.value("mode").toString();
    origin = request.value("origin").toString();
    account = request.value("account").toString();
    uid = request.value("uid").toString();
    if (account.isEmpty() || account.size() > 256 || origin.isEmpty() || origin.size() > 1024 ||
        (mode != "login" && mode != "enroll" && mode != "delete"))
        throw Failure(FaceAuthError::InvalidConfig, QStringLiteral("账号或任务参数无效"));
    FaceTemplateService templates(storage,config);
    if (mode == "delete") {
        templates.remove(origin, account);
        return QJsonObject{{"done",true},{"success",true},{"message",QStringLiteral("人脸模板已删除")}};
    }
    if (!config.enabled) throw Failure(FaceAuthError::Disabled, QStringLiteral("人脸功能尚未启用，请使用密码登录"));
    config.validate();
    if (mode == "login") saved = templates.load(origin, account);
    else if (uid.toInt() <= 0) throw Failure(FaceAuthError::InvalidConfig, QStringLiteral("请先完成服务器登录"));
    algorithm = makeAlgorithm(config);
    algorithm.camera->open(config.cameraIndex);
    started = true;
    elapsed.start(); sampling.start();
    return QJsonObject{{"message",QStringLiteral("摄像头已开启")},{"progress",0}};
}
QJsonObject FaceAuthSession::next(QImage *preview)
{
    if (!started) throw Failure(FaceAuthError::Internal, QStringLiteral("任务未启动"));
    if (elapsed.elapsed() > config.timeoutMs) throw Failure(FaceAuthError::Timeout, QStringLiteral("采集超时，请重试或使用密码登录"));
    QImage frame = algorithm.camera->read();
    QVector<Detection> detected = algorithm.detector->detect(frame);
    FaceQualityResult quality = inspectQuality(frame,detected,config);
    *preview = frame.copy();
    QString message = quality.message;
    if (mode == "enroll") message += "；" + enrollment.guidance();
    QJsonObject result{{"message",message},{"progress",enrollment.progress()}};
    if (config.debugMetrics) result["quality"] = QString("sharpness=%1 brightness=%2").arg(quality.sharpness,0,'f',0).arg(quality.brightness,0,'f',0);
    if (!quality.accepted() || sampling.elapsed() < 1000) { frame.fill(0); return result; }
    if (mode == "enroll" && !enrollment.acceptsPose(quality.yaw)) { frame.fill(0); return result; }
    QImage aligned = algorithm.aligner->align(frame,detected.first());
    frame.fill(0);
    QVector<float> embedding = algorithm.embedder->embed(aligned);
    aligned.fill(0);
    sampling.restart();
    if (mode == "enroll") {
        const bool accepted = enrollment.collect(embedding,quality.yaw,config.matchThreshold);
        if (!accepted) result["message"] = QStringLiteral("采集不一致，请保持同一人并重试");
        result["progress"] = enrollment.progress();
        if (enrollment.progress() == 3) {
            FaceTemplate value;
            value.userId = uid; value.account = account; value.origin = origin;
            value.modelId = config.modelId; value.modelVersion = config.modelVersion + ":" + config.embedderSha256;
            value.embedding = enrollment.finish();
            value.createdAt = value.updatedAt = QDateTime::currentDateTimeUtc();
            FaceTemplateService templates(storage,config);
            try { templates.save(value); } catch (...) { wipe(value.embedding); wipe(embedding); throw; }
            wipe(value.embedding);
            result["done"] = true; result["success"] = true;
            result["message"] = QStringLiteral("人脸注册成功，已安全保存特征模板");
        }
    } else {
        FaceAuthResult matched = login.verify(embedding,saved,config);
        result["message"] = matched.userMessage;
        result["attempts"] = login.attempts();
        if (matched.success || !matched.retryable) {
            result["done"] = true; result["success"] = matched.success;
        }
    }
    wipe(embedding);
    return result;
}
}
