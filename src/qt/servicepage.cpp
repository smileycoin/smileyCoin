//
// Created by Lenovo on 4/28/2020.
//

#include "servicepage.h"

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

//ServicePage::ServicePage(Mode mode, SubService subService, QWidget *parent) :
ServicePage::ServicePage(Mode mode, std::vector<std::tuple<std::string, std::string, std::string>> serviceObject, QWidget *parent) :
        QWidget(parent),
        model(0),
        mode(mode),
        serviceObject(serviceObject)
{
    QWidget *mainWindow = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetMaximumSize);
    mainWindow->setLayout(mainLayout);
    //ui->setupUi(this);
    setContentsMargins(0,0,0,0);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    serviceButton->setIcon(QIcon());
    ticketButton->setIcon(QIcon());
#endif

    switch(mode) {
        case ForConfirmingService: {
            formGroupBox = new QGroupBox(tr("This form is to confirm services"));

            serviceButton = new QPushButton(tr("Confirm New Service"));
            mainLayout->addWidget(formGroupBox);
            break;
        }
        case ForCreatingService: {
            formGroupBox = new QGroupBox(tr("This form is to create a new service"));
            QFormLayout *layout = new QFormLayout(this);

            serviceNameInput = new QValidatedLineEdit(this);
            serviceNameInput->setObjectName(QStringLiteral("serviceName"));
            layout->addRow(new QLabel(tr("Service Name:")), serviceNameInput);

            serviceAddressInput = new QValidatedLineEdit(this);
            serviceAddressInput->setObjectName(QStringLiteral("serviceAddress"));
            layout->addRow(new QLabel(tr("Service Address:")), serviceAddressInput);

            // Types of services
            serviceTypeCombo = new QComboBox(this);
            serviceTypeCombo->addItem("Ticket Sales");
            serviceTypeCombo->addItem("Universal Basic Income (UBI)");
            serviceTypeCombo->addItem("Book Chapter");
            serviceTypeCombo->addItem("Traceability");
            serviceTypeCombo->addItem("Nonprofit Organization");
            serviceTypeCombo->setObjectName(QStringLiteral("serviceTypeCombo"));
            layout->addRow(new QLabel(tr("Service Type:")), serviceTypeCombo);

            serviceButton = new QPushButton(tr("Create New Service"));
            serviceButton->setObjectName(QStringLiteral("serviceButton"));
            layout->addRow(serviceButton);

            formGroupBox->setLayout(layout);
            mainLayout->addWidget(formGroupBox);
            break;
        }

        case ForServiceOwner: {
            for(int i=0; i<serviceObject.size(); i++) {
                if (get<2>(serviceObject[i]) == "TicketSales") {
                    LogPrintStr("TicketSales i servicepage.cpp");

                    //formGroupBox = new QGroupBox(tr("Ticket Sales"));
                    formGroupBox = new QGroupBox(QString::fromStdString(get<0>(serviceObject[i])));
                    QFormLayout *layout = new QFormLayout(this);

                    ticketNameInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Ticket Name:")), ticketNameInput);

                    ticketDateInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Date:")), ticketDateInput);

                    ticketTimeInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Time:")), ticketTimeInput);

                    ticketLocInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Location:")), ticketLocInput);

                    ticketPriceInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Ticket Price:")), ticketPriceInput);

                    ticketAddressInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Ticket Address:")), ticketAddressInput);

                    ticketButton = new QPushButton(tr("Create Ticket"));
                    ticketButton->setObjectName(QStringLiteral("ticketButton"));
                    QIcon icon;
                    icon.addFile(QStringLiteral(":/icons/export"), QSize(), QIcon::Normal, QIcon::Off);
                    ticketButton->setIcon(icon);

                    layout->addRow(ticketButton);

                    formGroupBox->setLayout(layout);
                    mainLayout->addWidget(formGroupBox);
                }
                if (get<2>(serviceObject[i]) == "BookChapter") {
                    LogPrintStr("Book i servicepage.cpp");

                    //formGroupBox = new QGroupBox(tr("Book Chapter"));
                    formGroupBox = new QGroupBox(QString::fromStdString(get<0>(serviceObject[i])));
                    QFormLayout *layout = new QFormLayout(this);

                    serviceNameInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Book Title:")), serviceNameInput);

                    serviceAddressInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Book Chapter Address:")), serviceAddressInput);

                    ticketButton = new QPushButton(tr("Create Book Chapter"));
                    ticketButton->setObjectName(QStringLiteral("ticketButton"));
                    QIcon icon;
                    icon.addFile(QStringLiteral(":/icons/export"), QSize(), QIcon::Normal, QIcon::Off);
                    ticketButton->setIcon(icon);

                    layout->addRow(ticketButton);

                    formGroupBox->setLayout(layout);
                    mainLayout->addWidget(formGroupBox);
                }
                if (get<2>(serviceObject[i]) == "UBI") {
                    //formGroupBox = new QGroupBox(tr("UBI recipients"));
                    formGroupBox = new QGroupBox(QString::fromStdString(get<0>(serviceObject[i])));
                    QFormLayout *layout = new QFormLayout(this);

                    serviceNameInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Recipient Name:")), serviceNameInput);

                    serviceAddressInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Recipient Address:")), serviceAddressInput);

                    ticketButton = new QPushButton(tr("Create Recipient"));
                    ticketButton->setObjectName(QStringLiteral("ticketButton"));
                    QIcon icon;
                    icon.addFile(QStringLiteral(":/icons/export"), QSize(), QIcon::Normal, QIcon::Off);
                    ticketButton->setIcon(icon);

                    layout->addRow(ticketButton);

                    formGroupBox->setLayout(layout);
                    mainLayout->addWidget(formGroupBox);
                }
                if (get<2>(serviceObject[i]) == "NonprofitOrganization") {
                    //formGroupBox = new QGroupBox(tr("Nonprofit Organization"));
                    formGroupBox = new QGroupBox(QString::fromStdString(get<0>(serviceObject[i])));
                    QFormLayout *layout = new QFormLayout(this);

                    serviceNameInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Organization Name:")), serviceNameInput);

                    serviceAddressInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Organization Address:")), serviceAddressInput);

                    ticketButton = new QPushButton(tr("Create Nonprofit Organization"));
                    ticketButton->setObjectName(QStringLiteral("ticketButton"));
                    QIcon icon;
                    icon.addFile(QStringLiteral(":/icons/export"), QSize(), QIcon::Normal, QIcon::Off);
                    ticketButton->setIcon(icon);

                    layout->addRow(ticketButton);

                    formGroupBox->setLayout(layout);
                    mainLayout->addWidget(formGroupBox);
                }
                if (get<2>(serviceObject[i]) == "Traceability") {
                    //formGroupBox = new QGroupBox(tr("Traceability"));
                    formGroupBox = new QGroupBox(QString::fromStdString(get<0>(serviceObject[i])));
                    QFormLayout *layout = new QFormLayout(this);

                    serviceNameInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Traceability Name:")), serviceNameInput);

                    serviceAddressInput = new QValidatedLineEdit(this);
                    layout->addRow(new QLabel(tr("Traceability Address:")), serviceAddressInput);

                    ticketButton = new QPushButton(tr("Create Traceability"));
                    ticketButton->setObjectName(QStringLiteral("ticketButton"));
                    QIcon icon;
                    icon.addFile(QStringLiteral(":/icons/export"), QSize(), QIcon::Normal, QIcon::Off);
                    ticketButton->setIcon(icon);

                    layout->addRow(ticketButton);

                    formGroupBox->setLayout(layout);
                    mainLayout->addWidget(formGroupBox);
                }
            }

            newService = new QPushButton(tr("New Service"));
            newService->setObjectName(QStringLiteral("newService"));
            QIcon icon;
            icon.addFile(QStringLiteral(":/icons/add"), QSize(), QIcon::Normal, QIcon::Off);
            newService->setIcon(icon);
            mainLayout->addWidget(newService);

            break;
        }
    }

    // Connect signals for context menu actions
    connect(serviceButton, SIGNAL(clicked()), this, SLOT(onServiceAction()));
    connect(ticketButton, SIGNAL(clicked()), this, SLOT(onTicketAction()));
    connect(newService, SIGNAL(clicked()), this, SLOT(onNewServiceAction()));
}

void ServicePage::setModel(WalletModel *model) {
    this->model = model;
    if(!model)
        return;
}

void ServicePage::onServiceAction() {
    // Get new service name and convert to hex
    QString serviceName = serviceNameInput->text().toLatin1().toHex();
    // Get new service address and convert to hex
    QString serviceAddress = serviceAddressInput->text().toLatin1().toHex();
    // Get type of new service and convert to hex
    QString rawServiceType = serviceTypeCombo->currentText().toLatin1().toHex();
    std::vector<std::string> typeStr = splitString(rawServiceType.toStdString(), "20");
    // Merge into one string if service type name consists of more than one word
    QString serviceType = "";
    if (typeStr.size() > 1) {
        for (int i = 0; i < typeStr.size(); i++) {
            serviceType += QString::fromStdString(typeStr.at(i));
        }
    } else {
        serviceType = rawServiceType;
    }

    SendCoinsRecipient issuer;
    // Send new service request transaction to B9TRXJzgUJZZ5zPZbywtNfZHeu492WWRxc
    issuer.address = QString::fromStdString("B8dytMfspUhgMQUWGgdiR3QT8oUbNS9QVn");
    // Start with n = 10 (0.001) to get rid of spam
    issuer.amount = 1000000000;
    // Create op_return in the following form OP_RETURN = "new service serviceName serviceAddress serviceType"
    issuer.data = QString::fromStdString("6e6577207365727669636520") + serviceName +
                  QString::fromStdString("20") + serviceAddress + QString::fromStdString("20") + serviceType;

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
                                                               tr("Confirm new service"),
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

void ServicePage::onTicketAction() {
    QPushButton* buttonSender = qobject_cast<QPushButton*>(sender());
    QString buttonText = buttonSender->text();

    LogPrintStr("onTicketAction: " + buttonText.toStdString());

    QString ticketOpReturn = "";

    if (buttonText.toStdString() == "Create Ticket") {
        LogPrintStr("Create ticket buttonText ");
        QString rawTicketName = ticketNameInput->text().toLatin1().toHex();
        QString ticketDate = ticketDateInput->text().toLatin1().toHex();
        QString ticketTime = ticketTimeInput->text().toLatin1().toHex();
        QString rawTicketLoc = ticketLocInput->text().toLatin1().toHex();
        QString ticketPrice = ticketPriceInput->text().toLatin1().toHex();
        QString ticketAddress = ticketAddressInput->text().toLatin1().toHex();

        std::vector<std::string> nameStr = splitString(rawTicketName.toStdString(), "20");
        std::vector<std::string> locStr = splitString(rawTicketLoc.toStdString(), "20");

        // Merge into one string if ticket name or ticket location consists of more than one word
        QString ticketName = "";
        if (nameStr.size() > 1) {
            for (int i = 0; i < nameStr.size(); i++) {
                ticketName += QString::fromStdString(nameStr.at(i));
            }
        } else {
            ticketName = rawTicketName;
        }

        QString ticketLoc = "";
        if (locStr.size() > 1) {
            for (int i = 0; i < locStr.size(); i++) {
                ticketLoc += QString::fromStdString(locStr.at(i));
            }
        } else {
            ticketLoc = rawTicketLoc;
        }

        // Create op_return in the following form OP_RETURN = "new ticket ticketLoc ticketName ticketDate ticketTime ticketPrice ticketAddress"
        ticketOpReturn = QString::fromStdString("6e6577207469636b657420") +
                         ticketLoc + QString::fromStdString("20") +
                         ticketName + QString::fromStdString("20") +
                         ticketDate + QString::fromStdString("20") +
                         ticketTime + QString::fromStdString("20") +
                         ticketPrice + QString::fromStdString("20") +
                         ticketAddress;
    }

    if (buttonText.toStdString() == "Create Book Chapter") {

    }
    if (buttonText.toStdString() == "UBI") {

    }
    if (buttonText.toStdString() == "NonprofitOrganization") {

    }
    if (buttonText.toStdString() == "Traceability") {

    }

    SendCoinsRecipient issuer;
    // Send new service request transaction to B9TRXJzgUJZZ5zPZbywtNfZHeu492WWRxc
    issuer.address = QString::fromStdString("B8dytMfspUhgMQUWGgdiR3QT8oUbNS9QVn");
    // Start with n = 2 for ticket
    issuer.amount = 200000000;

    issuer.data = ticketOpReturn;

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
            accept();
            CoinControlDialog::coinControl->UnSelectAll();
        }
    }
}
void ServicePage::onNewServiceAction() {
}

void ServicePage::clear()
{
    serviceNameInput->clear();
    serviceAddressInput->clear();
}

void ServicePage::accept()
{
    clear();
}

void ServicePage::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    LogPrintStr("fer i processsendcoinsreturn");
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