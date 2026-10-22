#include "storage.h"
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#elif defined(Q_OS_MACOS)
#include <Security/Security.h>
#elif defined(FACE_WITH_SECRET)
#include <libsecret/secret.h>
#endif

namespace face {
static void unavailable()
{
    throw Failure(FaceAuthError::StorageUnavailable,
        QStringLiteral("系统安全存储不可用或已锁定；未启用明文降级。开发环境请安装并解锁系统密钥环"));
}
#ifdef Q_OS_WIN
static QString protectedPath(const QString &key)
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/face-templates";
    if (!QDir().mkpath(directory)) unavailable();
    return directory + "/" + key + ".dpapi";
}
QByteArray PlatformSecureStorageAdapter::read(const QString &key)
{
    QFile file(protectedPath(key));
    if (!file.exists()) return {};
    if (!file.open(QIODevice::ReadOnly) || file.size() > 65536) unavailable();
    QByteArray encrypted = file.readAll();
    DATA_BLOB input = {DWORD(encrypted.size()),reinterpret_cast<BYTE *>(encrypted.data())}, output = {};
    if (!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output)) unavailable();
    QByteArray plain(reinterpret_cast<char *>(output.pbData),int(output.cbData));
    SecureZeroMemory(output.pbData,output.cbData); LocalFree(output.pbData);
    return plain;
}
void PlatformSecureStorageAdapter::write(const QString &key, const QByteArray &payload)
{
    DATA_BLOB input = {DWORD(payload.size()),reinterpret_cast<BYTE *>(const_cast<char *>(payload.constData()))}, output = {};
    if (!CryptProtectData(&input,L"forChat biometric",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output)) unavailable();
    QByteArray encrypted(reinterpret_cast<char *>(output.pbData),int(output.cbData));
    SecureZeroMemory(output.pbData,output.cbData); LocalFree(output.pbData);
    QSaveFile file(protectedPath(key));
    if (!file.open(QIODevice::WriteOnly) || file.write(encrypted) != encrypted.size() || !file.commit()) unavailable();
}
void PlatformSecureStorageAdapter::remove(const QString &key)
{
    QFile file(protectedPath(key));
    if (file.exists() && !file.remove()) unavailable();
}
#elif defined(Q_OS_MACOS)
static const char serviceName[] = "org.forchat.biometric.v1";
QByteArray PlatformSecureStorageAdapter::read(const QString &key)
{
    QByteArray account = key.toLatin1();
    UInt32 size = 0;
    void *bytes = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(nullptr,sizeof(serviceName)-1,serviceName,
        UInt32(account.size()),account.constData(),&size,&bytes,nullptr);
    if (status == errSecItemNotFound) return {};
    if (status != errSecSuccess) unavailable();
    QByteArray result(static_cast<char *>(bytes),int(size));
    SecKeychainItemFreeContent(nullptr,bytes);
    return result;
}
void PlatformSecureStorageAdapter::write(const QString &key, const QByteArray &payload)
{
    QByteArray account = key.toLatin1();
    SecKeychainItemRef item = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(nullptr,sizeof(serviceName)-1,serviceName,
        UInt32(account.size()),account.constData(),nullptr,nullptr,&item);
    if (status == errSecSuccess) {
        status = SecKeychainItemModifyAttributesAndData(item,nullptr,UInt32(payload.size()),payload.constData());
        CFRelease(item);
    } else if (status == errSecItemNotFound) {
        status = SecKeychainAddGenericPassword(nullptr,sizeof(serviceName)-1,serviceName,
            UInt32(account.size()),account.constData(),UInt32(payload.size()),payload.constData(),nullptr);
    }
    if (status != errSecSuccess) unavailable();
}
void PlatformSecureStorageAdapter::remove(const QString &key)
{
    QByteArray account = key.toLatin1();
    SecKeychainItemRef item = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(nullptr,sizeof(serviceName)-1,serviceName,
        UInt32(account.size()),account.constData(),nullptr,nullptr,&item);
    if (status == errSecItemNotFound) return;
    if (status != errSecSuccess) unavailable();
    status = SecKeychainItemDelete(item); CFRelease(item);
    if (status != errSecSuccess) unavailable();
}
#elif defined(FACE_WITH_SECRET)
static const SecretSchema *schema()
{
    static const SecretSchema value = {
        "org.forchat.biometric.v1", SECRET_SCHEMA_NONE,
        {{"account",SECRET_SCHEMA_ATTRIBUTE_STRING},{nullptr,SECRET_SCHEMA_ATTRIBUTE_STRING}},
        0,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
    };
    return &value;
}
QByteArray PlatformSecureStorageAdapter::read(const QString &key)
{
    GError *error = nullptr;
    QByteArray account = key.toLatin1();
    gchar *secret = secret_password_lookup_sync(schema(),nullptr,&error,"account",account.constData(),nullptr);
    if (error) { g_error_free(error); unavailable(); }
    if (!secret) return {};
    QByteArray result(secret);
    secret_password_free(secret);
    return result;
}
void PlatformSecureStorageAdapter::write(const QString &key, const QByteArray &payload)
{
    GError *error = nullptr;
    QByteArray account = key.toLatin1();
    gboolean stored = secret_password_store_sync(schema(),SECRET_COLLECTION_DEFAULT,
        "forChat face template",payload.constData(),nullptr,&error,"account",account.constData(),nullptr);
    if (error) g_error_free(error);
    if (!stored) unavailable();
}
void PlatformSecureStorageAdapter::remove(const QString &key)
{
    GError *error = nullptr;
    QByteArray account = key.toLatin1();
    secret_password_clear_sync(schema(),nullptr,&error,"account",account.constData(),nullptr);
    if (error) { g_error_free(error); unavailable(); }
}
#else
QByteArray PlatformSecureStorageAdapter::read(const QString &) { unavailable(); return {}; }
void PlatformSecureStorageAdapter::write(const QString &, const QByteArray &) { unavailable(); }
void PlatformSecureStorageAdapter::remove(const QString &) { unavailable(); }
#endif
}
