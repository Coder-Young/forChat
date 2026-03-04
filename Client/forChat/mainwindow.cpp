#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->resize(300, 500);
    this->setWindowTitle("forChat");

    m_login_dlg = new LoginDialog(this);
    m_register_dlg = new RegisterDialog(this);

    this->setCentralWidget(m_login_dlg);

    m_login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    m_register_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    connect(m_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::slotSwitchReg);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotSwitchReg()
{
    this->setCentralWidget(m_register_dlg);
}

