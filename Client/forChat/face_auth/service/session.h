#pragma once
#include "algorithm/interfaces.h"
#include "storage/storage.h"
#include <QElapsedTimer>

namespace face {
class FaceTemplateService {
    IBiometricStorage &storage;
    FaceAuthConfig config;
public:
    FaceTemplateService(IBiometricStorage &backend, const FaceAuthConfig &settings)
        : storage(backend), config(settings) {}
    FaceTemplate load(const QString &origin, const QString &account);
    void save(const FaceTemplate &value);
    void remove(const QString &origin, const QString &account);
};
class FaceEnrollmentService {
    QVector<QVector<float>> samples;
public:
    ~FaceEnrollmentService();
    int progress() const { return samples.size(); }
    QString guidance() const;
    bool acceptsPose(double yaw) const;
    bool collect(QVector<float> embedding, double yaw, double threshold);
    QVector<float> finish() const;
    void clear();
};
class FaceLoginService {
    int failures = 0;
public:
    FaceAuthResult verify(const QVector<float> &probe, const FaceTemplate &saved,
                          const FaceAuthConfig &config);
    int attempts() const { return failures; }
};
class FaceAuthSession {
    FaceAuthConfig config;
    QString mode, origin, account, uid;
    IBiometricStorage &storage;
    Algorithm algorithm;
    FaceEnrollmentService enrollment;
    FaceLoginService login;
    FaceTemplate saved;
    QElapsedTimer elapsed, sampling;
    bool started = false;
public:
    FaceAuthSession(const FaceAuthConfig &settings, IBiometricStorage &backend);
    ~FaceAuthSession();
    QJsonObject start(const QJsonObject &request);
    QJsonObject next(QImage *preview);
    void cancel();
};
int runWorker();
}
