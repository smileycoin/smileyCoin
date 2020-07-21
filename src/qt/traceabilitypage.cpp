#include "traceabilitypage.h"

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

#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QIcon>
#include <QPushButton>

#include <qvalidatedlineedit.h>
#include <wallet.h>

TraceabilityPage::TraceabilityPage(Mode mode, QWidget *parent) :
        QWidget(parent),
        model(0),
        mode(mode)
{
    QWidget *mainWindow = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetMaximumSize);
    mainWindow->setLayout(mainLayout);
    //ui->setupUi(this);
    setContentsMargins(0,0,0,0);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    sendButton->setIcon(QIcon());
#endif

    switch(mode) {
        case ForConfirmingTransaction: {
            formGroupBox = new QGroupBox(tr("Confirm tracking transaction"));

            sendButton = new QPushButton(tr("Confirm tracking transaction"));
            mainLayout->addWidget(formGroupBox);
            break;
        }
        case ForCreatingTransaction: {
            formGroupBox = new QGroupBox(tr("Create new tracking transaction"));
            QFormLayout *layout = new QFormLayout(this);
            
            traceabilityAddressInput = new QValidatedLineEdit(this);
            traceabilityAddressInput->setObjectName(QStringLiteral("address"));
            layout->addRow(new QLabel(tr("Address:")), traceabilityAddressInput);

            traceabilityNumberInput = new QValidatedLineEdit(this);
            traceabilityNumberInput->setObjectName(QStringLiteral("number"));
            layout->addRow(new QLabel(tr("Number:")), traceabilityNumberInput);

            traceabilityAmountInput = new QValidatedLineEdit(this);
            traceabilityAmountInput->setObjectName(QStringLiteral("amount"));
            layout->addRow(new QLabel(tr("Amount:")), traceabilityAmountInput);

            traceabilityLocationInput = new QValidatedLineEdit(this);
            traceabilityLocationInput->setObjectName(QStringLiteral("location"));
            layout->addRow(new QLabel(tr("Location:")), traceabilityLocationInput);

            sendButton = new QPushButton(tr("Create tracking transaction"));
            sendButton->setObjectName(QStringLiteral("sendButton"));
            layout->addRow(sendButton);

            formGroupBox->setLayout(layout);
            mainLayout->addWidget(formGroupBox);
            break;
        }

            break;
        
    }

    // Connect signals for context menu actions
    connect(sendButton, SIGNAL(clicked()), this, SLOT(onTraceabilityAction()));
}

void TraceabilityPage::setModel(WalletModel *model) {
    this->model = model;
    if(!model)
        return;
}

void TraceabilityPage::onTraceabilityAction() {
    QString traceabilityAddress = traceabilityAddressInput->text();  //AUFPASSEN
    // Get new traceability amount and convert to hex
    QString traceabilityNumber = traceabilityNumberInput->text().toLatin1().toHex();
    // Get traceability amount and convert to hex
    QString traceabilityAmount = traceabilityAmountInput->text().toLatin1().toHex();
    // Get traceability location and convert to hex
    QString traceabilityLocation = traceabilityLocationInput->text().toLatin1().toHex();

    SendCoinsRecipient issuer;
    // Address transaction will be sent to
    issuer.address = traceabilityAddress;
    // Amount sent with transaction n = 10 (0.001)
    issuer.amount = 1000000000;
    // OP_RETURN = "Traceability traceabilityNumber traceabilityamount"
    issuer.data = QString::fromStdString("54726163656162696c697479") + QString::fromStdString("3b") + 
                  traceabilityNumber + QString::fromStdString("3b") + traceabilityAmount + 
                  QString::fromStdString("3b") + traceabilityLocation;

    QList <SendCoinsRecipient> recipients;
    recipients.append(issuer);

    // Format confirmation message
    QStringList formatted;
    // generate bold amount string
    QString amount =
            "<b>" + BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), issuer.amount);
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
                           BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                        currentTransaction.getTransactionFee()));


    qint64 txFee = currentTransaction.getTransactionFee();
    QString questionString = tr("Are you sure you want to create this tracking transaction?");
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
    foreach(BitcoinUnits::Unit
    u, BitcoinUnits::availableUnits())
    {
        if (u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1 (= %2)")
                                  .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                    totalAmount))
                                  .arg(alternativeUnits.join(" " + tr("or") + " ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this,
                                                               tr("Confirm tracking transaction"),
                                                               questionString.arg(formatted.join("<br />")),
                                                               QMessageBox::Yes | QMessageBox::Cancel,
                                                               QMessageBox::Cancel);

    if (retval == QMessageBox::Yes) {
        // now send the prepared transaction
        WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
        processSendCoinsReturn(sendStatus);

        if (sendStatus.status == WalletModel::OK) {
            accept();
            CoinControlDialog::coinControl->UnSelectAll();
        }
    }
}

void TraceabilityPage::clear()
{
    traceabilityNumberInput->clear();
    traceabilityAmountInput->clear();
}

void TraceabilityPage::accept()
{
    clear();
}

void TraceabilityPage::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    LogPrintStr("processSendcoinsReturn");
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
