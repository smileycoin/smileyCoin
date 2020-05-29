//
// Created by Lenovo on 4/28/2020.
//

#include "servicepage.h"
#include "ui_servicepage.h"

#include "addresstablemodel.h"
#include "bitcoingui.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "guiutil.h"
#include "coincontroldialog.h"
#include "base58.h"
#include "coincontrol.h"
#include "sendcoinsentry.h"
#include "sendcoinsdialog.h"


#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

#include <wallet.h>


ServicePage::ServicePage(Mode mode, QWidget *parent) :
        QWidget(parent),
        ui(new Ui::ServicePage),
        model(0),
        mode(mode)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->serviceButton->setIcon(QIcon());
#endif

    switch(mode)
    {
        case ForConfirmingService:
            ui->serviceExplanation->setText(tr("Your address is either the genesis address or an official issuance address."));
            ui->serviceButton->setText(tr("Confirm New Service"));
            break;
        case ForCreatingService:
            ui->serviceExplanation->setText(tr("This window is for creating a new service."));
            ui->serviceButton->setText(tr("Create New Service"));
            sendCoinsDialog = new SendCoinsDialog();
            break;
    }

    // Connect signals for context menu actions
    connect(ui->serviceButton, SIGNAL(clicked()), this, SLOT(onServiceAction()));
}

ServicePage::~ServicePage()
{
    delete ui;
}

void ServicePage::setAddress(const QString &address)
{
    LogPrintStr("Set address");
    //ui->textarea->setText(address);
}

void ServicePage::setModel(WalletModel *model) {
    this->model = model;
    if(!model)
        return;
}

void ServicePage::onServiceAction() {
    switch(mode)
    {
        case ForConfirmingService:
            ui->serviceName->setText("Confirm New Service Input");
            break;
        case ForCreatingService:
            //ui->textarea->setText("Create New Service Input");

            // Get new service name and convert to hex
            QString serviceName = ui->serviceName->text().toLatin1().toHex();
            // Get new service address and convert to hex
            QString serviceAddress = ui->serviceAddress->text().toLatin1().toHex();

            SendCoinsRecipient issuer;
            // Send new service request transaction to B9TRXJzgUJZZ5zPZbywtNfZHeu492WWRxc
            issuer.address = QString::fromStdString("B8dytMfspUhgMQUWGgdiR3QT8oUbNS9QVn");
            // Start with n = 10 (0.001) to get rid of spam
            issuer.amount = 1000000000;
            // Create op_return in the following form OP_RETURN = "new service serviceName serviceAddress"
            issuer.data = QString::fromStdString("6e6577207365727669636520") + serviceName +
                    QString::fromStdString("20") + serviceAddress;

            QList<SendCoinsRecipient> recipients;
            recipients.append(issuer);

            // Format confirmation message
            QStringList formatted;
            // generate bold amount string
            QString amount = "<b>" + BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), issuer.amount);
            amount.append("</b>");

            // generate monospace address string
            QString address = "<span style='font-family: monospace;'>" + issuer.address;
            address.append("</span>");

            QString recipientElement = tr("%1 to %2").arg(amount, address);

            formatted.append(recipientElement);

            WalletModelTransaction currentTransaction(recipients);
            WalletModel::SendCoinsReturn prepareStatus;
            if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
                prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
            else
                prepareStatus = model->prepareTransaction(currentTransaction);

            // process prepareStatus and on error generate message shown to user
            processSendCoinsReturn(prepareStatus,
                                   BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));


            CWalletTx *newTx = currentTransaction.getTransaction();

            qint64 txFee = currentTransaction.getTransactionFee();
            QString questionString = tr("Are you sure you want to create a new service?");
            questionString.append("<br /><br />%1");

            if(txFee > 0)
            {
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
                if(u != model->getOptionsModel()->getDisplayUnit())
                    alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
            }
            questionString.append(tr("Total Amount %1 (= %2)")
                                          .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
                                          .arg(alternativeUnits.join(" " + tr("or") + " ")));

            QMessageBox::StandardButton retval = QMessageBox::question(this,
                    tr("Confirm new service"),
                    questionString.arg(formatted.join("<br />")),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Cancel);

            if(retval == QMessageBox::Yes)
            {
                // now send the prepared transaction
                WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
                processSendCoinsReturn(sendStatus);

                if (sendStatus.status == WalletModel::OK)
                {
                    CoinControlDialog::coinControl->UnSelectAll();
                }
            }

            break;
    }

}

void ServicePage::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
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
            msgParams.first = tr("The recipient address is not valid, please recheck.");
            break;
        case WalletModel::InvalidAmount:
            msgParams.first = tr("The amount to pay must be larger than 0.");
            break;
        case WalletModel::AmountExceedsBalance:
            msgParams.first = tr("The amount exceeds your balance.");
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            break;
        case WalletModel::DuplicateAddress:
            msgParams.first = tr("Duplicate address found, can only send to each address once per send operation.");
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