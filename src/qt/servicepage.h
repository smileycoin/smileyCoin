//
// Created by Lenovo on 4/28/2020.
//

#ifndef SERVICEPAGE_H
#define SERVICEPAGE_H

#include "walletmodel.h"

#include <QWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class WalletModel;
class OptionsModel;
class SendCoinsDialog;
class QValidatedLineEdit;

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
        ForCreatingService,
        ForServiceOwner
    };

    //explicit ServicePage(Mode mode, SubService subService, QWidget *parent);
    explicit ServicePage(Mode mode, std::vector<std::tuple<std::string, std::string, std::string>> serviceObject, QWidget *parent);

    void setModel(WalletModel *model);

public slots:
    void clear();
    void accept();

private:
    Ui::ServicePage *ui;
    WalletModel *model;
    Mode mode;
    //SubService subService;
    std::vector<std::tuple<std::string, std::string, std::string>> serviceObject;

    SendCoinsDialog *sendCoinsDialog;

    QGroupBox *formGroupBox;
    QValidatedLineEdit *serviceNameInput;
    QValidatedLineEdit *serviceAddressInput;

    QValidatedLineEdit *ticketNameInput;
    QValidatedLineEdit *ticketDateInput;
    QValidatedLineEdit *ticketTimeInput;
    QValidatedLineEdit *ticketLocInput;
    QValidatedLineEdit *ticketPriceInput;
    QValidatedLineEdit *ticketAddressInput;

    QComboBox *serviceTypeCombo;
    QPushButton *serviceButton;
    QPushButton *newService;
    QPushButton *ticketButton;

    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onServiceAction();

private slots:
    void onTicketAction();

};

#endif //SERVICEPAGE_H
