#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    void on_Register_get_Btn_clicked();
    void slot_reg_mod_finish(ReqID req_id, QString res, ErrCode err);

private:
    void showTips(QString str, bool b_ok);
    void initHttpHandler();

private:
    Ui::RegisterDialog *ui;

    QMap<ReqID, std::function<void(const QJsonObject&)>> _handlers;
};

#endif // REGISTERDIALOG_H
