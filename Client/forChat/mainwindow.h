#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "logindialog.h"
#include "registerdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:
    void slotSwitchReg();

private:
    Ui::MainWindow *ui;

    LoginDialog*        m_login_dlg;
    RegisterDialog*     m_register_dlg;
};
#endif // MAINWINDOW_H
