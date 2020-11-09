//
// Created by Lenovo on 4/28/2020.
//

#include "servicepage.h"
#include "ui_servicepage.h"

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
#include "init.h"
#include "editservicedialog.h"
#include "servicetablemodel.h"
#include "transactiondescdialog.h"

#include <qvalidatedlineedit.h>
#include <wallet.h>
#include <QSortFilterProxyModel>
#include <QIcon>

ServicePage::ServicePage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ServicePage),
    serviceModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    /*ServiceList.GetMyServiceAddresses(myServices);
    if (myServices.empty()) {
        ui->viewAllServices->hide();
        ui->viewMyServices->hide();
    } else {
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ui->viewAllServices->sizePolicy().hasHeightForWidth());
        ui->viewAllServices->setSizePolicy(sizePolicy);
        ui->viewAllServices->setChecked(true);

        sizePolicy.setHeightForWidth(ui->viewMyServices->sizePolicy().hasHeightForWidth());
        ui->viewMyServices->setSizePolicy(sizePolicy);
        ui->viewMyServices->setChecked(false);

        ui->horizontalLayout->setAlignment(ui->viewAllServices, Qt::AlignHCenter);
        ui->horizontalLayout->setAlignment(ui->viewMyServices, Qt::AlignHCenter);
    }
    ui->deleteService->hide();*/

    connect(ui->newService, SIGNAL(clicked()), this, SLOT(onNewServiceAction()));
    connect(ui->deleteService, SIGNAL(clicked()), this, SLOT(onDeleteServiceAction()));
}

ServicePage::~ServicePage()
{
    delete ui;
}

void ServicePage::setServiceModel(ServiceTableModel *serviceModel) {
    this->serviceModel = serviceModel;
    if(!serviceModel)
        return;
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(serviceModel);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(ServiceTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(ServiceTableModel::Address, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(ServiceTableModel::Type, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(ServiceTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ServiceTableModel::Address, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ServiceTableModel::Type, QHeaderView::ResizeToContents);
#endif

    // Select row for newly created address
    connect(serviceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));
}

void ServicePage::setWalletModel(WalletModel *walletModel) {
    this->walletModel = walletModel;
    if(!walletModel)
        return;
}

void ServicePage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(ServiceTableModel::Address);

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

void ServicePage::onNewServiceAction() {
    if(!walletModel)
        return;

    EditServiceDialog dlg(EditServiceDialog::NewService, this);
    connect(&dlg, SIGNAL(accepted()), this, SLOT(EditServiceDialog::accept()));
    dlg.setModel(walletModel);
    dlg.exec();
}

void ServicePage::onDeleteServiceAction() {
    if(!walletModel)
        return;

    if(!ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        QModelIndex idx = selection.at(0);
        int row = idx.row();

        QString serviceName = idx.sibling(row, 0).data().toString();
        QString serviceAddress = idx.sibling(row, 1).data().toString();

        CBitcoinAddress sAddress = CBitcoinAddress(serviceAddress.toStdString());
        if (!IsMine(*pwalletMain, sAddress.Get())) {
            QMessageBox::warning(this, windowTitle(),
                    tr("Permission denied. The service address \"%1\" does not belong to this wallet.").arg(serviceAddress),
                    QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        SendCoinsRecipient issuer;
        // Send delete service transaction to own service address
        issuer.address = serviceAddress;
        // Start with n = 1 to get rid of spam
        issuer.amount = 1*COIN;

        // Create op_return in the following form OP_RETURN = "DS serviceAddress"
        issuer.data = QString::fromStdString("445320") + serviceAddress.toLatin1().toHex();

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

        QString deleteString = tr("Are you sure you want to delete \"%1\"?").arg(serviceName);
        deleteString.append("<br /><br />");

        deleteString.append("<span style='font-weight:normal;'>");
        deleteString.append("This service will be deleted when the transaction has been confirmed. You can't undo this action.");
        deleteString.append("</span> ");

        QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, tr("Confirm Service Deletion"),
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

void ServicePage::onViewAllServices()
{
    ui->deleteService->hide();

    serviceModel = new ServiceTableModel(true, pwalletMain, walletModel);
    setServiceModel(serviceModel);
}

void ServicePage::onViewMyServices()
{
    ui->deleteService->show();

    serviceModel = new ServiceTableModel(false, pwalletMain, walletModel);
    setServiceModel(serviceModel);
}

void ServicePage::showServiceDetails()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog dlg(selection.at(0));
        dlg.exec();
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