//
// Created by Lenovo on 6/30/2020.
//

#include "couponpage.h"
#include "ui_couponpage.h"

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
#include "coupontablemodel.h"
#include "transactiondescdialog.h"

#include <qvalidatedlineedit.h>
#include <wallet.h>
#include <QSortFilterProxyModel>
#include <QIcon>
#include <QPainter>
#include <QStylePainter>

CouponPage::CouponPage(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::CouponPage),
        couponModel(0),
        walletModel(0)
{
    ui->setupUi(this);

    // Add current services to dropdown
    ServiceList.GetServiceAddresses(allServices);
    ui->couponService->addItem("All");
    for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = allServices.begin(); it!=allServices.end(); it++ )
    {
        // If service type is Coupon Sales add to dropdown selection
        if(get<2>(it->second) == "Coupon Sales") {
            ui->couponService->addItem(QString::fromStdString(get<1>(it->second)));
        }
    }

    ServiceList.GetMyServiceAddresses(myServices);
    if (myServices.empty()) {
        ui->newCoupon->hide();
        ui->deleteCoupon->hide();
    } else {
        ui->newCoupon->show();
        ui->deleteCoupon->show();
    }
    ui->serviceLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Connect signals for context menu actions
    connect(ui->newCoupon, SIGNAL(clicked()), this, SLOT(onNewCouponAction()));
    connect(ui->deleteCoupon, SIGNAL(clicked()), this, SLOT(onDeleteCouponAction()));
    connect(ui->couponService, SIGNAL(activated(QString)), this, SLOT(chooseService(QString)));
    connect(ui->buyCoupon, SIGNAL(clicked()), this, SLOT(onBuyCouponAction()));
}

CouponPage::~CouponPage()
{
    delete ui;
}

void CouponPage::setCouponModel(CouponTableModel *couponModel) {
    this->couponModel = couponModel;
    if(!couponModel)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(couponModel);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->tableView->setModel(proxyModel);

    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(CouponTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(CouponTableModel::Location, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(CouponTableModel::DateTime, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(CouponTableModel::Price, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(CouponTableModel::Service, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(CouponTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(CouponTableModel::Location, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(CouponTableModel::DateTime, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(CouponTableModel::Price, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(CouponTableModel::Address, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(CouponTableModel::Service, QHeaderView::ResizeToContents);
#endif

    // Select row for newly created address
    connect(couponModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));
}

void CouponPage::setWalletModel(WalletModel *walletModel) {
    this->walletModel = walletModel;
    if(!walletModel)
        return;
}

void CouponPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(CouponTableModel::Address);

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

void CouponPage::onNewCouponAction() {
    if(!walletModel)
        return;

    EditServiceDialog dlg(EditServiceDialog::NewCoupon, this);
    connect(&dlg, SIGNAL(accepted()), this, SLOT(EditServiceDialog::accept()));
    dlg.setModel(walletModel);
    dlg.exec();
}

void CouponPage::onDeleteCouponAction() {
    if(!walletModel)
        return;

    if(!ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        QModelIndex idx = selection.at(0);
        int row = idx.row();

        QString couponName = idx.sibling(row, 0).data().toString();
        QString couponAddress = idx.sibling(row, 4).data().toString();
        QString couponService = idx.sibling(row, 5).data().toString();

        std::string serviceAddress = "";
        ServiceList.GetServiceAddresses(allServices);
        for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = allServices.begin(); it!=allServices.end(); it++ )
        {
            if (couponService.toStdString() == get<1>(it->second)) {
                serviceAddress = it->first;
            }
        }

        CBitcoinAddress sAddress = CBitcoinAddress(serviceAddress);
        CBitcoinAddress tAddress = CBitcoinAddress(couponAddress.toStdString());
        if (!IsMine(*pwalletMain, sAddress.Get()) || !IsMine(*pwalletMain, tAddress.Get())) {
            QMessageBox::warning(this, windowTitle(),
                    tr("Permission denied. Neither the coupon address \"%1\" nor the service address \"%2\" belong to this wallet.").arg(couponAddress).arg(QString::fromStdString(serviceAddress)),
                    QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        SendCoinsRecipient issuer;
        // Send delete service transaction to own coupon address
        issuer.address = couponAddress;
        // Start with n = 1 to get rid of spam
        issuer.amount = 1*COIN;

        // Create op_return in the following form OP_RETURN = "DT couponAddress"
        issuer.data = QString::fromStdString("445420") + couponAddress.toLatin1().toHex();

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

        QString deleteString = tr("Are you sure you want to delete \"%1\"?").arg(couponName);
        deleteString.append("<br /><br />");

        deleteString.append("<span style='font-weight:normal;'>");
        deleteString.append("This coupon will be deleted when the transaction has been confirmed. You can't undo this action.");
        deleteString.append("</span> ");

        QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, tr("Confirm Coupon Deletion"),
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

void CouponPage::onBuyCouponAction() {
    if(!walletModel)
        return;

    if(!ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        QModelIndex idx = selection.at(0);
        int row = idx.row();

        QString couponName = idx.sibling(row, 0).data().toString();
        QString couponValue = idx.sibling(row, 3).data().toString();
        QString couponAddress = idx.sibling(row, 4).data().toString();
        QString couponService = idx.sibling(row, 5).data().toString();

        CBitcoinAddress tAddress = CBitcoinAddress(couponAddress.toStdString());

        SendCoinsRecipient issuer;
        // Send delete service transaction to own coupon address
        issuer.address = couponAddress;
        // Pay coupon amount to coupon address
        issuer.amount = couponValue.toInt()*COIN;

        // Create op_return in the following form OP_RETURN = "BT couponAddress"
        issuer.data = QString::fromStdString("425420") + couponAddress.toLatin1().toHex();

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

        QString buyString = tr("Are you sure you want to buy \"%1\"?").arg(couponName);
        buyString.append("<br /><br />");

        buyString.append("<span style='font-weight:normal;'>");
        buyString.append("This coupon will be deleted when the transaction has been confirmed. You can't undo this action.");
        buyString.append("</span> ");

        QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, tr("Confirm Coupon Purchase"),
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

void CouponPage::chooseService(QString text) {
    couponModel = new CouponTableModel(text.toStdString(), pwalletMain, walletModel);
    setCouponModel(couponModel);
}

void CouponPage::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
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