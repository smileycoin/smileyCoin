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
#include "init.h"
#include "editservicedialog.h"
#include "tickettablemodel.h"
#include "transactiondescdialog.h"

#include <qvalidatedlineedit.h>
#include <wallet.h>
#include <QSortFilterProxyModel>
#include <QIcon>

TicketPage::TicketPage(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::TicketPage),
        ticketModel(0),
        walletModel(0)
{
    ui->setupUi(this);

    ServiceList.GetMyServiceAddresses(myServices);
    if (myServices.empty()) {
        ui->newTicket->hide();
        ui->deleteTicket->hide();
    }

    // Connect signals for context menu actions
    connect(ui->newTicket, SIGNAL(clicked()), this, SLOT(onNewTicketAction()));
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
    ui->tableView->verticalHeader()->setDefaultSectionSize(50);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Location, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::DateTime, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Price, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(TicketTableModel::Address, QHeaderView::Stretch);
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
