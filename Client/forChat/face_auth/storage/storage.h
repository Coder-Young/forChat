#pragma once
#include "core/types.h"
#include <memory>

namespace face {
class IBiometricStorage {
public:
    virtual ~IBiometricStorage() {}
    virtual QByteArray read(const QString &key) = 0;
    virtual void write(const QString &key, const QByteArray &payload) = 0;
    virtual void remove(const QString &key) = 0;
};
class PlatformSecureStorageAdapter : public IBiometricStorage {
public:
    QByteArray read(const QString &key) override;
    void write(const QString &key, const QByteArray &payload) override;
    void remove(const QString &key) override;
};
using SecureBiometricStorage = PlatformSecureStorageAdapter;
QByteArray serializeTemplate(const FaceTemplate &value);
FaceTemplate parseTemplate(const QByteArray &data, const QString &origin, const QString &account,
                           const FaceAuthConfig &config);
}
