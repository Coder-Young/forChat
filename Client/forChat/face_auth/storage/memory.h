#pragma once
#include "storage.h"
#include <QHash>
namespace face {
class MemoryBiometricStorage : public IBiometricStorage {
    QHash<QString,QByteArray> entries;
public:
    ~MemoryBiometricStorage() override {
        for (auto entry = entries.begin(); entry != entries.end(); ++entry) wipe(entry.value());
    }
    QByteArray read(const QString &key) override { return entries.value(key); }
    void write(const QString &key, const QByteArray &payload) override {
        if (entries.contains(key)) wipe(entries[key]);
        entries[key] = payload;
    }
    void remove(const QString &key) override {
        if (entries.contains(key)) wipe(entries[key]);
        entries.remove(key);
    }
};
}
