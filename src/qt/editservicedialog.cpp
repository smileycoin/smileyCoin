//
// Created by Lenovo on 6/15/2020.
//

#include "editservicedialog.h"
#include "ui_editservicedialog.h"

#include "walletmodel.h"
#include "bitcoingui.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "base58.h"
#include "guiconstants.h"
#include "coincontroldialog.h"
#include "coincontrol.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "sendcoinsdialog.h"
#include "init.h"
#include "servicelistdb.h"
#include "serviceitemlistdb.h"
#include "util.h"

#include <qvalidatedlineedit.h>
#include <wallet.h>


EditServiceDialog::EditServiceDialog(Mode mode, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::EditServiceDialog),
        mode(mode),
        model(0)
{
    ui->setupUi(this);

    switch(mode)
    {
        case NewService:
        {
            ui->couponForm->hide();
            ui->serviceForm->show();
            setWindowTitle(tr("Create new service"));
            ui->serviceType->addItem("Coupon Sales");
            ui->serviceType->addItem("UBI");
            ui->serviceType->addItem("Book Chapter");
            ui->serviceType->addItem("Traceability");
            ui->serviceType->addItem("Nonprofit Organization");
            ui->serviceType->addItem("DEX");
            ui->serviceType->addItem("Survey");

            ui->serviceName->setMaxLength(25);
            ui->sCounterName->setText("25 characters left");

            break;
        }
        case NewCoupon:
        {
            ui->couponDateTime->setDate(QDate::currentDate());
            ui->serviceForm->hide();
            ui->couponForm->show();
            setWindowTitle(tr("Create new coupon"));

            ServiceList.GetMyServiceAddresses(myServices);
            for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
            {
                // If service type is Coupon Sales add to dropdown selection
                if(get<2>(it->second) == "Coupon Sales") {
                    ui->couponService->addItem(QString::fromStdString(get<1>(it->second)));
                }
            }
            ui->couponName->setMaxLength(20);
            ui->tCounterName->setText("20 characters left");

            ui->couponLocation->setMaxLength(20);
            ui->tCounterLoc->setText("20 characters left");

            break;
        }
    }

    connect(ui->serviceName, SIGNAL(textChanged(const QString &)), this, SLOT(sNameCount(const QString &)));
    connect(ui->couponName, SIGNAL(textChanged(const QString &)), this, SLOT(tNameCount(const QString &)));
    connect(ui->couponLocation, SIGNAL(textChanged(const QString &)), this, SLOT(tLocationCount(const QString &)));

    connect(ui->serviceName, SIGNAL(textChanged(const QString &)), this, SLOT(valueChanged(const QString &)));
    connect(ui->serviceAddress, SIGNAL(textChanged(const QString &)), this, SLOT(valueChanged(const QString &)));
    connect(ui->couponName, SIGNAL(textChanged(const QString &)), this, SLOT(valueChanged(const QString &)));
    connect(ui->couponLocation, SIGNAL(textChanged(const QString &)), this, SLOT(valueChanged(const QString &)));
    connect(ui->couponPrice, SIGNAL(textChanged(const QString &)), this, SLOT(valueChanged(const QString &)));
    connect(ui->couponAddress, SIGNAL(textChanged(const QString &)), this, SLOT(valueChanged(const QString &)));
}

EditServiceDialog::~EditServiceDialog()
{
    delete ui;
}

void EditServiceDialog::setModel(WalletModel *model)
{
    this->model = model;
    if(!model)
        return;
}

void EditServiceDialog::accept()
{
    if(!model)
        return;

     switch(mode)
     {
         case NewService:
         {
             if (ui->serviceName->text().isEmpty() || ui->serviceAddress->text().isEmpty()) {
                 if (ui->serviceName->text().isEmpty()) {
                     ui->serviceName->setStyleSheet(STYLE_INVALID);
                 }
                 if (ui->serviceAddress->text().isEmpty()) {
                     ui->serviceAddress->setStyleSheet(STYLE_INVALID);
                 }
                 return;
             }

             CBitcoinAddress sAddress = CBitcoinAddress(ui->serviceAddress->text().toStdString());
             if (!sAddress.IsValid()) {
                 QMessageBox::warning(this, windowTitle(),
                         tr("The entered address \"%1\" is not a valid Smileycoin address.").arg(ui->serviceAddress->text()),
                         QMessageBox::Ok, QMessageBox::Ok);
                 return;
             }

             if (ServiceList.IsService(ui->serviceAddress->text().toStdString())) {
                 QMessageBox::warning(this, windowTitle(),
                         tr("The entered address \"%1\" is already on service list. Please use another address.").arg(ui->serviceAddress->text()),
                         QMessageBox::Ok, QMessageBox::Ok);
                 return;
             }

             // Get new service name and convert to hex
             QString rawServiceName = ui->serviceName->text().toLatin1().toHex();
             // Get new service address and convert to hex
             QString serviceAddress = ui->serviceAddress->text().toLatin1().toHex();
             // Get type of new service and convert to hex
             QString rawServiceType = ui->serviceType->currentText();
             QString serviceType = "";

             if (rawServiceType == "Coupon Sales") {
                 serviceType = "31";
             } else if (rawServiceType == "UBI") {
                 serviceType = "32";
             } else if (rawServiceType == "Book Chapter") {
                 serviceType = "33";
             } else if (rawServiceType == "Traceability") {
                 serviceType = "34";
             } else if (rawServiceType == "Nonprofit Organization") {
                 serviceType = "35";
             } else if (rawServiceType == "DEX") {
                 serviceType = "36";
             } else if (rawServiceType == "Survey") {
                 serviceType = "37";
             }

             std::vector<std::string> nameStr = splitString(rawServiceName.toStdString(), "20");
             // Merge into one string if service name consists of more than one word
             QString serviceName = "";
             if (nameStr.size() > 1) {
                 for (std::string::size_type i = 0; i < nameStr.size(); i++) {
                     serviceName += QString::fromStdString(nameStr.at(i));
                 }
             } else {
                 serviceName = rawServiceName;
             }

             SendCoinsRecipient issuer;
             // Send new service request transaction to official service address
             issuer.address = QString::fromStdString("B9TRXJzgUJZZ5zPZbywtNfZHeu492WWRxc");

             // Start with n = 10 (0.001) to get rid of spam
             issuer.amount = 10*COIN;

             // Create op_return in the following form OP_RETURN = "NS serviceName serviceAddress serviceType"
             issuer.data = QString::fromStdString("4e5320") + serviceName +
                           QString::fromStdString("20") + serviceAddress + QString::fromStdString("20") + serviceType;

             QList <SendCoinsRecipient> recipients;
             recipients.append(issuer);

             // Format confirmation message
             QStringList formatted;

             // generate bold amount string
             QString amount = "<b>" + BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), issuer.amount);
             amount.append("</b>");

             // generate monospace address string
             QString address2 = "<span style='font-family: monospace;'>" + issuer.address;
             address2.append("</span>");

             QString recipientElement = tr("%1 to %2").arg(amount, address2);

             formatted.append(recipientElement);

             WalletModelTransaction currentTransaction(recipients);
             WalletModel::SendCoinsReturn prepareStatus;
             if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
                 prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
             else
                 prepareStatus = model->prepareTransaction(currentTransaction);

             // process prepareStatus and on error generate message shown to user
             processSendCoinsReturn(prepareStatus,
                                    BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                 currentTransaction.getTransactionFee()));

             qint64 txFee = currentTransaction.getTransactionFee();
             QString questionString = tr("Are you sure you want to create a new service?");
             questionString.append("<br /><br />%1");

             if (txFee > 0) {
                 // append fee string if a fee is required
                 questionString.append("<hr /><span style='color:#aa0000;'>");
                 questionString.append(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
                 questionString.append("</span> ");
                 questionString.append(tr("added as transaction fee"));
             }

             // add total amount in all subdivision units
             questionString.append("<hr />");
             qint64 totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
             QStringList alternativeUnits;
             foreach(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
             {
                 if (u != model->getOptionsModel()->getDisplayUnit())
                     alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
             }
             questionString.append(tr("Total Amount %1 (= %2)")
                                           .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                             totalAmount))
                                           .arg(alternativeUnits.join(" " + tr("or") + " ")));

             QMessageBox::StandardButton retval = QMessageBox::question(this,
                                                                        tr("Confirm new service"),
                                                                        questionString.arg(formatted.join("<br />")),
                                                                        QMessageBox::Yes | QMessageBox::Cancel,
                                                                        QMessageBox::Cancel);

             if (retval == QMessageBox::Yes) {
                 // now send the prepared transaction
                 WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
                 processSendCoinsReturn(sendStatus);

                 if (sendStatus.status == WalletModel::OK) {
                     QDialog::accept();
                     CoinControlDialog::coinControl->UnSelectAll();
                 }
             }

             break;
         }
         case NewCoupon:
         {
             if (ui->couponName->text().isEmpty() || ui->couponLocation->text().isEmpty() || ui->couponPrice->text().isEmpty() || ui->couponAddress->text().isEmpty())
             {
                 if (ui->couponName->text().isEmpty()) {
                     ui->couponName->setStyleSheet(STYLE_INVALID);
                 }
                 if (ui->couponLocation->text().isEmpty()) {
                     ui->couponLocation->setStyleSheet(STYLE_INVALID);
                 }
                 if (ui->couponPrice->text().isEmpty()) {
                     ui->couponPrice->setStyleSheet(STYLE_INVALID);
                 }
                 if (ui->couponAddress->text().isEmpty()) {
                     ui->couponAddress->setStyleSheet(STYLE_INVALID);
                 }
                 return;
             }

             CBitcoinAddress tAddress = CBitcoinAddress(ui->couponAddress->text().toStdString());
             if (!tAddress.IsValid()) {
                 QMessageBox::warning(this, windowTitle(),
                         tr("The entered address \"%1\" is not a valid Smileycoin address.").arg(ui->couponAddress->text()),
                         QMessageBox::Ok, QMessageBox::Ok);
                 return;
             }
             if (!is_number(ui->couponPrice->text().toStdString())) {
                 QMessageBox::warning(this, windowTitle(), "Coupon price must be a number.", QMessageBox::Ok,
                         QMessageBox::Ok);
                 return;
             }
             // Don't allow new coupon if date and time has already expired
             if (!is_before(ui->couponDateTime->dateTime().toString("dd/MM/yyyyhh:mm").toStdString())) {
                 QMessageBox::warning(this, windowTitle(),
                         tr("The entered coupon date and time \"%1\" has already expired.").arg(ui->couponDateTime->dateTime().toString("dd/MM/yyyyhh:mm")),
                         QMessageBox::Ok, QMessageBox::Ok);
                 return;
             }
             if (ServiceItemList.IsCoupon(ui->couponAddress->text().toStdString())) {
                 QMessageBox::warning(this, windowTitle(),
                         tr("The entered address \"%1\" is already on coupon list. Please use another address.").arg(ui->couponAddress->text()),
                         QMessageBox::Ok, QMessageBox::Ok);
                 return;
             }

             QString rawCouponService = ui->couponService->currentText();
             QString rawCouponLoc = ui->couponLocation->text().toLatin1().toHex();
             QString rawCouponName = ui->couponName->text().toLatin1().toHex();
             QString couponDateTime = ui->couponDateTime->dateTime().toString("dd/MM/yyyyhh:mm").toLatin1().toHex();
             QString couponPrice = ui->couponPrice->text().toLatin1().toHex();
             QString couponAddress = ui->couponAddress->text().toLatin1().toHex();

             std::vector<std::string> nameStr = splitString(rawCouponName.toStdString(), "20");
             std::vector<std::string> locStr = splitString(rawCouponLoc.toStdString(), "20");

             // Merge into one string if coupon name or coupon location consists of more than one word
             QString couponName = "";
             if (nameStr.size() > 1) {
                 for (std::string::size_type i = 0; i < nameStr.size(); i++) {
                     couponName += QString::fromStdString(nameStr.at(i));
                 }
             } else {
                 couponName = rawCouponName;
             }

             QString couponLoc = "";
             if (locStr.size() > 1) {
                 for (std::string::size_type i = 0; i < locStr.size(); i++) {
                     couponLoc += QString::fromStdString(locStr.at(i));
                 }
             } else {
                 couponLoc = rawCouponLoc;
             }


             QString couponServiceAddress = "";
             SendCoinsRecipient issuer;
             // Send new coupon transaction to corresponding service address
             for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
             {
                 if(rawCouponService == QString::fromStdString(get<1>(it->second))) {
                     couponServiceAddress = QString::fromStdString(it->first);
                 }
             }
             issuer.address = couponServiceAddress;
             // Start with n = 1 to get rid of spam
             issuer.amount = 1*COIN;

             // Create op_return in the following form OP_RETURN = "NT couponLoc couponName couponDateTime couponPrice couponAddress"
             issuer.data = QString::fromStdString("4e5420") +
                              couponLoc + QString::fromStdString("20") +
                              couponName + QString::fromStdString("20") +
                              couponDateTime + QString::fromStdString("20") +
                              couponPrice + QString::fromStdString("20") +
                              couponAddress;

             QList <SendCoinsRecipient> recipients;
             recipients.append(issuer);

             // Format confirmation message
             QStringList formatted;

             // generate bold amount string
             QString amount = "<b>" + BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), issuer.amount);
             amount.append("</b>");

             // generate monospace address string
             QString address2 = "<span style='font-family: monospace;'>" + issuer.address;
             address2.append("</span>");

             QString recipientElement = tr("%1 to %2").arg(amount, address2);

             formatted.append(recipientElement);

             WalletModelTransaction currentTransaction(recipients);
             WalletModel::SendCoinsReturn prepareStatus;
             if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
                 prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
             else
                 prepareStatus = model->prepareTransaction(currentTransaction);

             // process prepareStatus and on error generate message shown to user
             processSendCoinsReturn(prepareStatus,
                                    BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                 currentTransaction.getTransactionFee()));

             qint64 txFee = currentTransaction.getTransactionFee();
             QString questionString = tr("Are you sure you want to create a new coupon?");
             questionString.append("<br /><br />%1");

             if (txFee > 0) {
                 // append fee string if a fee is required
                 questionString.append("<hr /><span style='color:#aa0000;'>");
                 questionString.append(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
                 questionString.append("</span> ");
                 questionString.append(tr("added as transaction fee"));
             }

             // add total amount in all subdivision units
             questionString.append("<hr />");
             qint64 totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
             QStringList alternativeUnits;
             foreach(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
             {
                 if (u != model->getOptionsModel()->getDisplayUnit())
                     alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
             }
             questionString.append(tr("Total Amount %1 (= %2)")
                                           .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                             totalAmount))
                                           .arg(alternativeUnits.join(" " + tr("or") + " ")));

             QMessageBox::StandardButton retval = QMessageBox::question(this,
                                                                        tr("Confirm new coupon"),
                                                                        questionString.arg(formatted.join("<br />")),
                                                                        QMessageBox::Yes | QMessageBox::Cancel,
                                                                        QMessageBox::Cancel);

             if (retval == QMessageBox::Yes) {
                 // now send the prepared transaction
                 WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
                 processSendCoinsReturn(sendStatus);

                 if (sendStatus.status == WalletModel::OK) {
                     QDialog::accept();
                     CoinControlDialog::coinControl->UnSelectAll();
                 } else {
                     QDialog::reject();
                 }
             }

             break;
         }
     }
}

void EditServiceDialog::sNameCount(const QString & text) {
    QString text_label = QString("%1 characters left").arg(ui->serviceName->maxLength() - text.size());
    ui->sCounterName->setText(text_label);
}

void EditServiceDialog::tNameCount(const QString & text) {
    QString text_label = QString("%1 characters left").arg(ui->couponName->maxLength() - text.size());
    ui->tCounterName->setText(text_label);
}

void EditServiceDialog::tLocationCount(const QString & text) {
    QString text_label = QString("%1 characters left").arg(ui->couponLocation->maxLength() - text.size());
    ui->tCounterLoc->setText(text_label);
}

void EditServiceDialog::valueChanged(const QString & text) {
    QString text_label = QString("%1 characters left").arg(ui->couponLocation->maxLength() - text.size());
    ui->tCounterLoc->setText(text_label);
}

void EditServiceDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
        case WalletModel::InvalidAddress:
            msgParams.first = tr("The entered address \"%1\" is not a valid Smileycoin address.");
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            break;
        case WalletModel::TransactionCreationFailed:
            msgParams.first = tr("Transaction creation failed!");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::TransactionCommitFailed:
            msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
            // included to prevent a compiler warning.
        case WalletModel::OK:
        default:
            return;
    }
}