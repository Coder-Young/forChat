#include "dialog.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

namespace face {
FaceAuthSettingsWidget::FaceAuthSettingsWidget(QWidget *parent) : QLabel(parent)
{
    setWordWrap(true);
    setText(QStringLiteral("当前版本仅适合本地演示，不具备完整的照片攻击和视频攻击防护能力。\n"
                           "只保存特征模板；本地验证后仍需密码认证。点击开始即同意访问摄像头。"));
}
FaceDialog::FaceDialog(bool enrollment, const QString &server, const QString &user,
                       const QString &userId, QWidget *parent)
    : QDialog(parent), enrollmentMode(enrollment), origin(server), account(user), uid(userId),
      config(FaceAuthConfig::load())
{
    setWindowTitle(enrollment ? QStringLiteral("注册 / 管理人脸") : QStringLiteral("人脸登录（本地验证）"));
    setMinimumWidth(540);
    auto *layout = new QVBoxLayout(this);
    preview = new FaceCameraPreview(this);
    quality = new FaceQualityIndicator(this);
    cameraStatus = new QLabel(QStringLiteral("摄像头已关闭"),this);
    settings = new FaceAuthSettingsWidget(this);
    progress = new QProgressBar(this);
    progress->setRange(0,3); progress->setVisible(enrollmentMode);
    startButton = new QPushButton(enrollment ? QStringLiteral("开始注册 / 重新采集") : QStringLiteral("开始验证 / 重试"),this);
    deleteButton = new QPushButton(QStringLiteral("删除已有模板"),this);
    deleteButton->setVisible(enrollmentMode);
    cancelButton = new QPushButton(enrollment ? QStringLiteral("取消注册") : QStringLiteral("取消并使用密码登录"),this);
    layout->addWidget(settings); layout->addWidget(preview); layout->addWidget(cameraStatus);
    layout->addWidget(quality); layout->addWidget(progress);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(startButton); buttons->addWidget(deleteButton); buttons->addWidget(cancelButton);
    layout->addLayout(buttons);
    timeout.setSingleShot(true); stopTimer.setSingleShot(true);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setStandardErrorFile(QProcess::nullDevice());
    connect(startButton,&QPushButton::clicked,this,[this]() {
        start(enrollmentMode ? "enroll" : "login");
    });
    connect(deleteButton,&QPushButton::clicked,this,[this]() {
        if (QMessageBox::question(this,QStringLiteral("删除人脸"),QStringLiteral("确认删除此账号的本地人脸模板？")) == QMessageBox::Yes)
            start("delete");
    });
    connect(cancelButton,&QPushButton::clicked,this,&FaceDialog::reject);
    connect(&process,&QProcess::started,this,[this]() {
        send(QJsonObject{{"op","start"},{"mode",process.property("mode").toString()},
            {"origin",origin},{"account",account},{"uid",uid}});
    });
    connect(&process,&QProcess::readyReadStandardOutput,this,&FaceDialog::read);
    connect(&process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,[this](int,QProcess::ExitStatus) {
        settle();
    });
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            quality->setText(QStringLiteral("无法启动人脸任务，请使用密码登录"));
            settle();
        }
    });
    connect(&timeout,&QTimer::timeout,this,[this]() {
        quality->setText(QStringLiteral("操作超时，已停止摄像头；可重试或使用密码登录"));
        stop();
    });
    connect(&stopTimer,&QTimer::timeout,this,[this]() { process.kill(); });
    connect(qApp,&QCoreApplication::aboutToQuit,this,[this]() {
        process.kill();
        process.waitForFinished(1500);
        preview->clear();
        wipe(incoming);
    });
}
FaceDialog::~FaceDialog()
{
    process.kill();
    process.waitForFinished(1500);
    wipe(incoming);
    preview->clear();
}
void FaceDialog::send(const QJsonObject &request)
{
    QByteArray data = QJsonDocument(request).toJson(QJsonDocument::Compact);
    quint32 size = quint32(data.size());
    QByteArray header;
    header.append(char(size >> 24)); header.append(char(size >> 16));
    header.append(char(size >> 8)); header.append(char(size));
    process.write(header); process.write(data);
}
void FaceDialog::start(const QString &mode)
{
    if (busy) return;
    if (!enrollmentMode && launches >= config.maxRetries) {
        quality->setText(QStringLiteral("本次窗口重试次数已用尽，请使用密码登录"));
        return;
    }
    if (mode != "delete") {
        try { config.validate(); }
        catch (const Failure &failure) { quality->setText(failure.message); return; }
    }
    ++launches; busy = true; closing = false; verified = false;
    wipe(incoming); progress->setValue(0);
    startButton->setEnabled(false); deleteButton->setEnabled(false);
    cameraStatus->setText(mode == "delete" ? QStringLiteral("正在访问系统安全存储") : QStringLiteral("正在准备摄像头与模型"));
    quality->clear();
    process.setProperty("mode",mode);
    process.start(QCoreApplication::applicationFilePath(),QStringList{"--face-auth-worker"});
    timeout.start(qBound(1000,config.timeoutMs,120000));
}
void FaceDialog::read()
{
    incoming.append(process.readAllStandardOutput());
    if (incoming.size() > 1024 * 1024) {
        quality->setText(QStringLiteral("工作进程响应无效")); stop(); return;
    }
    if (incoming.size() < 4) return;
    const auto *header = reinterpret_cast<const uchar *>(incoming.constData());
    quint32 size = (quint32(header[0]) << 24) | (quint32(header[1]) << 16) | (quint32(header[2]) << 8) | header[3];
    if (size > 1024 * 1024 - 4) { stop(); return; }
    if (incoming.size() < int(size) + 4) return;
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(incoming.mid(4,int(size)),&error);
    wipe(incoming);
    if (closing) return;
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        quality->setText(QStringLiteral("工作进程响应无效")); stop(); return;
    }
    QJsonObject result = document.object();
    quality->setText(result.value("message").toString());
    if (config.debugMetrics && result.contains("quality")) quality->setText(quality->text() + "\n" + result.value("quality").toString());
    progress->setValue(result.value("progress").toInt());
    if (result.value("done").toBool()) {
        verified = result.value("success").toBool() && !enrollmentMode;
        timeout.stop();
        preview->clear();
        cameraStatus->setText(QStringLiteral("摄像头已关闭"));
        return;
    }
    const QByteArray imageBytes = QByteArray::fromBase64(result.value("preview").toString().toLatin1());
    if (!imageBytes.isEmpty()) {
        QImage image = QImage::fromData(imageBytes,"JPEG");
        if (!image.isNull() && image.width() <= 640 && image.height() <= 480) {
            preview->setPixmap(QPixmap::fromImage(image));
            cameraStatus->setText(QStringLiteral("摄像头已开启"));
        }
        image.fill(0);
    }
    QTimer::singleShot(60,this,[this]() {
        if (busy && !closing && process.state() == QProcess::Running) send(QJsonObject{{"op","next"}});
    });
}
void FaceDialog::stop()
{
    closing = true;
    timeout.stop();
    preview->clear();
    cameraStatus->setText(QStringLiteral("正在关闭摄像头"));
    if (process.state() == QProcess::NotRunning) { settle(); return; }
    send(QJsonObject{{"op","cancel"}});
    stopTimer.start(300);
}
void FaceDialog::settle()
{
    timeout.stop(); stopTimer.stop(); busy = false;
    preview->clear(); wipe(incoming);
    cameraStatus->setText(QStringLiteral("摄像头已关闭"));
    startButton->setEnabled(enrollmentMode || launches < config.maxRetries);
    deleteButton->setEnabled(true);
    if (closing) QDialog::reject();
    else if (verified) QDialog::accept();
}
void FaceDialog::reject()
{
    if (!busy) { QDialog::reject(); return; }
    stop();
}
}
