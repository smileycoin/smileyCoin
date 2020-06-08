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

    explicit ServicePage(Mode mode, std::vector<std::tuple<std::string, std::string, std::string>> serviceObject, QWidget *parent);

    void setModel(WalletModel *model);

public slots:
    void clear();
    void accept();

private:
    WalletModel *model;
    Mode mode;
    std::vector<std::tuple<std::string, std::string, std::string>> serviceObject;

    SendCoinsDialog *sendCoinsDialog;

    QGroupBox *formGroupBox;

    QValidatedLineEdit *serviceNameInput;
    QValidatedLineEdit *serviceAddressInput;
    QComboBox *serviceTypeCombo;
    QPushButton *serviceButton;

    QValidatedLineEdit *ticketNameInput;
    QValidatedLineEdit *ticketDateInput;
    QValidatedLineEdit *ticketTimeInput;
    QValidatedLineEdit *ticketLocInput;
    QValidatedLineEdit *ticketPriceInput;
    QValidatedLineEdit *ticketAddressInput;
    QPushButton *ticketButton;

    QPushButton *newService;

    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onServiceAction();
    void onTicketAction();
    void onNewServiceAction();

};

#endif //SERVICEPAGE_H
