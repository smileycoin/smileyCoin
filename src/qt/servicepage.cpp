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

    ServiceList.GetMyServiceAddresses(myServices);
    if (myServices.empty()) {
        ui->viewAllServices->hide();
        ui->viewMyServices->hide();
        ui->deleteService->hide();
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

    connect(ui->newService, SIGNAL(clicked()), this, SLOT(onNewServiceAction()));
    connect(ui->deleteService, SIGNAL(clicked()), this, SLOT(onDeleteServiceAction()));
    connect(ui->viewAllServices, SIGNAL(toggled(bool)), this, SLOT(onViewAllServices()));
    connect(ui->viewMyServices, SIGNAL(toggled(bool)), this, SLOT(onViewMyServices()));
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
    ui->tableView->verticalHeader()->setDefaultSectionSize(50);

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
    LogPrintStr("onDeleteServiceAction");

    if(!walletModel)
        return;

    /*EditServiceDialog dlg(EditServiceDialog::DeleteService, this);
    connect(&dlg, SIGNAL(accepted()), this, SLOT(EditServiceDialog::accept()));
    dlg.setModel(walletModel);
    dlg.exec();*/

    if(!ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    QModelIndex idx = selection.at(0);

    if(!selection.isEmpty())
    {
        QString name = idx.data(ServiceTableModel::Name).toString();
        QString address = idx.data(ServiceTableModel::Address).toString();
        QString type = idx.data(ServiceTableModel::Type).toString();

        LogPrintStr("name: "+name.toStdString()+" address: "+address.toStdString()+" type: "+type.toStdString());
        //TransactionDescDialog dlg(selection.at(0));
        //dlg.exec();
    }
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
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog dlg(selection.at(0));
        dlg.exec();
    }
}