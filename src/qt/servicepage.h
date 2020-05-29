//
// Created by Lenovo on 4/28/2020.
//

#ifndef SERVICEPAGE_H
#define SERVICEPAGE_H

#include "walletmodel.h"

#include <QWidget>

class WalletModel;
class OptionsModel;
class SendCoinsDialog;

namespace Ui {
    class ServicePage;
}

/** Widget that shows a list of sending or receiving addresses.
  */
class ServicePage : public QWidget
{
    Q_OBJECT

public:
    enum Mode {
        ForConfirmingService,
        ForCreatingService
    };

    explicit ServicePage(Mode mode, QWidget *parent);
    ~ServicePage();

    void setModel(WalletModel *model);
    void setAddress(const QString &address);

private:
    Ui::ServicePage *ui;
    WalletModel *model;
    Mode mode;
    SendCoinsDialog *sendCoinsDialog;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onServiceAction();

};

#endif //SERVICEPAGE_H
