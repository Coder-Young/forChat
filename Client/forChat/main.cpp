/*****************************************************************
 *
 * @file    main.cpp
 * @brief   主界面
 *
 * @author  yangzhiqi
 * @data    2026/02/27
 *****************************************************************/

#include "mainwindow.h"

#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qss(":/style/stylesheet.qss");
    if (qss.open(QFile::ReadOnly))
    {
        qDebug("qss Open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }
    else
    {
        qDebug("qss Open failed");
    }

    QString fileName = "config.ini";
    QString appPath = QCoreApplication::applicationDirPath();
    QString config_path = QDir::toNativeSeparators(appPath + QDir::separator() + "config" + QDir::separator() + fileName);
    QSettings settings(config_path, QSettings::IniFormat);

    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
