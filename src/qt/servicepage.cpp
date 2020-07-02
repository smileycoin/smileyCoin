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
    serviceUi(new Ui::ServicePage),
    serviceModel(0),
    walletModel(0)
{
    serviceUi->setupUi(this);

    ServiceList.GetMyServiceAddresses(myServices);
    if (myServices.empty()) {
        serviceUi->viewAllServices->hide();
        serviceUi->viewMyServices->hide();
        serviceUi->deleteService->hide();
    } else {
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(serviceUi->viewAllServices->sizePolicy().hasHeightForWidth());
        serviceUi->viewAllServices->setSizePolicy(sizePolicy);
        serviceUi->viewAllServices->setChecked(true);

        sizePolicy.setHeightForWidth(serviceUi->viewMyServices->sizePolicy().hasHeightForWidth());
        serviceUi->viewMyServices->setSizePolicy(sizePolicy);
        serviceUi->viewMyServices->setChecked(false);

        serviceUi->horizontalLayout->setAlignment(serviceUi->viewAllServices, Qt::AlignHCenter);
        serviceUi->horizontalLayout->setAlignment(serviceUi->viewMyServices, Qt::AlignHCenter);
    }

    connect(serviceUi->newService, SIGNAL(clicked()), this, SLOT(onNewServiceAction()));
    connect(serviceUi->viewAllServices, SIGNAL(toggled(bool)), this, SLOT(onViewAllServices()));
    connect(serviceUi->viewMyServices, SIGNAL(toggled(bool)), this, SLOT(onViewMyServices()));
}

ServicePage::~ServicePage()
{
    delete serviceUi;
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

    serviceUi->tableView->setModel(proxyModel);
    serviceUi->tableView->sortByColumn(0, Qt::AscendingOrder);
    serviceUi->tableView->verticalHeader()->setDefaultSectionSize(50);

    // Set column widths
#if QT_VERSION < 0x050000
    serviceUi->tableView->horizontalHeader()->setResizeMode(ServiceTableModel::Name, QHeaderView::ResizeToContents);
    serviceUi->tableView->horizontalHeader()->setResizeMode(ServiceTableModel::Address, QHeaderView::Stretch);
    serviceUi->tableView->horizontalHeader()->setResizeMode(ServiceTableModel::Type, QHeaderView::ResizeToContents);
#else
    serviceUi->tableView->horizontalHeader()->setSectionResizeMode(ServiceTableModel::Name, QHeaderView::ResizeToContents);
    serviceUi->tableView->horizontalHeader()->setSectionResizeMode(ServiceTableModel::Address, QHeaderView::Stretch);
    serviceUi->tableView->horizontalHeader()->setSectionResizeMode(ServiceTableModel::Type, QHeaderView::ResizeToContents);
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
    QTableView *table = serviceUi->tableView;
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

void ServicePage::onViewAllServices()
{
    serviceModel = new ServiceTableModel(true, pwalletMain, walletModel);
    setServiceModel(serviceModel);
}

void ServicePage::onViewMyServices()
{
    serviceModel = new ServiceTableModel(false, pwalletMain, walletModel);
    setServiceModel(serviceModel);
}

void ServicePage::showServiceDetails()
{
    if(!serviceUi->tableView->selectionModel())
        return;
    QModelIndexList selection = serviceUi->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog dlg(selection.at(0));
        dlg.exec();
    }
}