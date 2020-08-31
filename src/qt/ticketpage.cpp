//
// Created by Lenovo on 6/30/2020.
//

#include "ticketpage.h"
#include "ui_ticketpage.h"

#include "bitcoingui.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "guiutil.h"
#include "coincontroldialog.h"
#include "base58.h"
#include "coincontrol.h"
#include "sendcoinsentry.h"
#include "sendcoinsdialog.h"
#include "servicelistdb.h"
#include "serviceitemlistdb.h"
#include "init.h"
#include "editservicedialog.h"
#include "tickettablemodel.h"
#include "transactiondescdialog.h"

#include <qvalidatedlineedit.h>
#include <wallet.h>
#include <QSortFilterProxyModel>
#include <QIcon>
#include <QPainter>
#include <QStylePainter>

TicketPage::TicketPage(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::TicketPage),
        ticketModel(0),
        walletModel(0)
{
    ui->setupUi(this);

    // Add current services to dropdown
    ServiceList.GetServiceAddresses(allServices);
    ui->ticketService->addItem("All");
    for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = allServices.begin(); it!=allServices.end(); it++ )
    {
        // If service type is Ticket Sales add to dropdown selection
        if(get<2>(it->second) == "Ticket Sales") {
            ui->ticketService->addItem(QString::fromStdString(get<1>(it->second)));
        }
    }

    ServiceList.GetMyServiceAddresses(myServices);
    if (myServices.empty()) {
        ui->newTicket->hide();
        ui->deleteTicket->hide();
    } else {
        ui->newTicket->show();
        ui->deleteTicket->show();
    }
    ui->serviceLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Connect signals for context menu actions
    connect(ui->newTicket, SIGNAL(clicked()), this, SLOT(onNewTicketAction()));
    connect(ui->deleteTicket, SIGNAL(clicked()), this, SLOT(onDeleteTicketAction()));
    connect(ui->ticketService, SIGNAL(activated(QString)), this, SLOT(chooseService(QString)));
}

TicketPage::~TicketPage()
{
    delete ui;
}

void TicketPage::setTicketModel(TicketTableModel *ticketModel) {
    this->ticketModel = ticketModel;
    if(!ticketModel)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(ticketModel);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->tableView->setModel(proxyModel);

    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Location, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::DateTime, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Price, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Service, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(TicketTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(TicketTableModel::Location, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(TicketTableModel::DateTime, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(TicketTableModel::Price, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(TicketTableModel::Address, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(TicketTableModel::Service, QHeaderView::ResizeToContents);
#endif

    // Select row for newly created address
    connect(ticketModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));
    //connect(buyButton, SIGNAL(clicked()), this, SLOT(onBuyClicked()));
}

void TicketPage::setWalletModel(WalletModel *walletModel) {
    this->walletModel = walletModel;
    if(!walletModel)
        return;
}

void TicketPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(TicketTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void TicketPage::onNewTicketAction() {
    if(!walletModel)
        return;

    EditServiceDialog dlg(EditServiceDialog::NewTicket, this);
    connect(&dlg, SIGNAL(accepted()), this, SLOT(EditServiceDialog::accept()));
    dlg.setModel(walletModel);
    dlg.exec();
}

void TicketPage::onDeleteTicketAction() {
    if(!walletModel)
        return;

    if(!ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        QModelIndex idx = selection.at(0);
        int row = idx.row();

        QString ticketName = idx.sibling(row, 0).data().toString();
        QString ticketAddress = idx.sibling(row, 4).data().toString();
        QString ticketService = idx.sibling(row, 5).data().toString();

        std::string serviceAddress = "";
        ServiceList.GetServiceAddresses(allServices);
        for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = allServices.begin(); it!=allServices.end(); it++ )
        {
            if (ticketService.toStdString() == get<1>(it->second)) {
                serviceAddress = it->first;
            }
        }

        CBitcoinAddress sAddress = CBitcoinAddress(serviceAddress);
        CBitcoinAddress tAddress = CBitcoinAddress(ticketAddress.toStdString());
        if (!IsMine(*pwalletMain, sAddress.Get()) || !IsMine(*pwalletMain, tAddress.Get())) {
            QMessageBox::warning(this, windowTitle(),
                    tr("Permission denied. Neither the ticket address \"%1\" nor the service address \"%2\" belong to this wallet.").arg(ticketAddress).arg(QString::fromStdString(serviceAddress)),
                    QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        SendCoinsRecipient issuer;
        // Send delete service transaction to own ticket address
        issuer.address = ticketAddress;
        // Start with n = 1 to get rid of spam
        issuer.amount = 1*COIN;

        // Create op_return in the following form OP_RETURN = "DT ticketAddress"
        issuer.data = QString::fromStdString("445420") + ticketAddress.toLatin1().toHex();

        QList <SendCoinsRecipient> recipients;
        recipients.append(issuer);

        WalletModelTransaction currentTransaction(recipients);
        WalletModel::SendCoinsReturn prepareStatus;
        if (walletModel->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
            prepareStatus = walletModel->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
        else
            prepareStatus = walletModel->prepareTransaction(currentTransaction);

        // process prepareStatus and on error generate message shown to user
        processSendCoinsReturn(prepareStatus,
                BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),
                        currentTransaction.getTransactionFee()));

        QString deleteString = tr("Are you sure you want to delete \"%1\"?").arg(ticketName);
        deleteString.append("<br /><br />");

        deleteString.append("<span style='font-weight:normal;'>");
        deleteString.append("This ticket will be deleted when the transaction has been confirmed. You can't undo this action.");
        deleteString.append("</span> ");

        QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, tr("Confirm Ticket Deletion"),
                deleteString, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if (confirmDelete == QMessageBox::Yes) {
            // now send the prepared transaction
            WalletModel::SendCoinsReturn sendStatus = walletModel->sendCoins(currentTransaction);
            processSendCoinsReturn(sendStatus);

            if (sendStatus.status == WalletModel::OK) {
                QDialog::accept();
                CoinControlDialog::coinControl->UnSelectAll();
            } else {
                QDialog::reject();
            }
        }
    }
}

void TicketPage::onBuyTicketAction() {
    if(!walletModel)
        return;

    if(!ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        QModelIndex idx = selection.at(0);
        int row = idx.row();

        QString ticketName = idx.sibling(row, 0).data().toString();
        QString ticketValue = idx.sibling(row, 3).data().toString();
        QString ticketAddress = idx.sibling(row, 4).data().toString();
        QString ticketService = idx.sibling(row, 5).data().toString();

        CBitcoinAddress tAddress = CBitcoinAddress(ticketAddress.toStdString());

        SendCoinsRecipient issuer;
        // Send delete service transaction to own ticket address
        issuer.address = ticketAddress;
        // Pay ticket amount to ticket address
        issuer.amount = ticketValue.toInt()*COIN;

        // Create op_return in the following form OP_RETURN = "BT ticketAddress"
        issuer.data = QString::fromStdString("425420") + ticketAddress.toLatin1().toHex();

        QList <SendCoinsRecipient> recipients;
        recipients.append(issuer);

        WalletModelTransaction currentTransaction(recipients);
        WalletModel::SendCoinsReturn prepareStatus;
        if (walletModel->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
            prepareStatus = walletModel->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
        else
            prepareStatus = walletModel->prepareTransaction(currentTransaction);

        // process prepareStatus and on error generate message shown to user
        processSendCoinsReturn(prepareStatus,
                BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),
                        currentTransaction.getTransactionFee()));

        QString buyString = tr("Are you sure you want to buy \"%1\"?").arg(ticketName);
        buyString.append("<br /><br />");

        buyString.append("<span style='font-weight:normal;'>");
        buyString.append("This ticket will be deleted when the transaction has been confirmed. You can't undo this action.");
        buyString.append("</span> ");

        QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, tr("Confirm Ticket Purchase"),
                buyString, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if (confirmDelete == QMessageBox::Yes) {
            // now send the prepared transaction
            WalletModel::SendCoinsReturn sendStatus = walletModel->sendCoins(currentTransaction);
            processSendCoinsReturn(sendStatus);

            if (sendStatus.status == WalletModel::OK) {
                QDialog::accept();
                CoinControlDialog::coinControl->UnSelectAll();
            } else {
                QDialog::reject();
            }
        }
    }
}

void TicketPage::chooseService(QString text) {
    ticketModel = new TicketTableModel(text.toStdString(), pwalletMain, walletModel);
    setTicketModel(ticketModel);
}

void TicketPage::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
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