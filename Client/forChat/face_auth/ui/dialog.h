#pragma once
#include "core/types.h"
#include <QDialog>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>

namespace face {
class FaceCameraPreview : public QLabel {
public:
    explicit FaceCameraPreview(QWidget *parent) : QLabel(parent) {
        setMinimumSize(480,300); setAlignment(Qt::AlignCenter); setText(QStringLiteral("摄像头已关闭"));
    }
};
class FaceQualityIndicator : public QLabel {
public:
    explicit FaceQualityIndicator(QWidget *parent) : QLabel(parent) { setWordWrap(true); }
};
class FaceAuthSettingsWidget : public QLabel {
public:
    explicit FaceAuthSettingsWidget(QWidget *parent);
};
class FaceDialog : public QDialog {
    Q_OBJECT
public:
    FaceDialog(bool enrollment, const QString &origin, const QString &account,
               const QString &uid, QWidget *parent = nullptr);
    ~FaceDialog() override;
    void reject() override;
private:
    bool enrollmentMode, verified = false, closing = false, busy = false;
    int launches = 0;
    QString origin, account, uid;
    FaceAuthConfig config;
    QProcess process;
    QTimer timeout, stopTimer;
    QByteArray incoming;
    FaceCameraPreview *preview;
    FaceQualityIndicator *quality;
    QLabel *cameraStatus;
    FaceAuthSettingsWidget *settings;
    QProgressBar *progress;
    QPushButton *startButton, *deleteButton, *cancelButton;
    void start(const QString &mode);
    void send(const QJsonObject &request);
    void read();
    void stop();
    void settle();
};
class FaceEnrollmentDialog : public FaceDialog {
public:
    FaceEnrollmentDialog(const QString &origin, const QString &account, const QString &uid, QWidget *parent)
        : FaceDialog(true,origin,account,uid,parent) {}
};
class FaceLoginWidget : public FaceDialog {
public:
    FaceLoginWidget(const QString &origin, const QString &account, QWidget *parent)
        : FaceDialog(false,origin,account,QString(),parent) {}
};
}
