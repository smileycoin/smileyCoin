//
// Created by Lenovo on 7/3/2020.
//

#include "tickettablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet.h"
#include "servicelistdb.h"

#include <QFont>

struct TicketTableEntry
{
    QString name;
    QString location;
    QString datetime;
    QString price;
    QString address;
    QString service;

    TicketTableEntry() {}
    TicketTableEntry(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service):
            name(name), location(location), datetime(datetime), price(price), address(address), service(service) {}
};

struct TicketTableEntryLessThan
{
    bool operator()(const TicketTableEntry &a, const TicketTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const TicketTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const TicketTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class TicketTablePriv
{
public:
    CWallet *wallet;
    QList<TicketTableEntry> cachedTicketTable;
    TicketTableModel *parent;


    TicketTablePriv(CWallet *wallet, TicketTableModel *parent):
            wallet(wallet), parent(parent) {}

    void refreshTicketTable()
    {
        LogPrintStr("refreshTicketTable");
        cachedTicketTable.clear();
        {
            LogPrintStr("cachedtickettable");
            //LOCK(wallet->cs_wallet);
            std::multiset<std::pair< CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> retset;
            ServiceList.GetServiceAddressInfo(retset);
            for(std::multiset< std::pair< CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > >::const_iterator
                        it = retset.begin(); it!=retset.end(); it++ )
            {
                cachedTicketTable.append(TicketTableEntry(QString::fromStdString(get<2>(it->second)),
                                                            QString::fromStdString(get<1>(it->second)),
                                                            QString::fromStdString(get<3>(it->second)),
                                                            QString::fromStdString(get<4>(it->second)),
                                                            QString::fromStdString(get<5>(it->second)),
                                                            QString::fromStdString(get<0>(it->second))));

            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedTicketTable.begin(), cachedTicketTable.end(), TicketTableEntryLessThan());
    }

    void updateTicketEntry(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service, int status)
    {
        LogPrintStr(" updateeeeEntry2-tickettablemodel ");
        // Find address / label in model
        QList<TicketTableEntry>::iterator lower = qLowerBound(
                cachedTicketTable.begin(), cachedTicketTable.end(), address, TicketTableEntryLessThan());
        QList<TicketTableEntry>::iterator upper = qUpperBound(
                cachedTicketTable.begin(), cachedTicketTable.end(), address, TicketTableEntryLessThan());
        int lowerIndex = (lower - cachedTicketTable.begin());
        int upperIndex = (upper - cachedTicketTable.begin());
        bool inModel = (lower != upper);
        //ServiceTableEntry::Type newEntryType = translateTransactionType(purpose, isMine);

        switch(status)
        {
            case CT_NEW:
            {
                if(inModel)
                {
                    break;
                }
                parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);

                // Birta service address nafn i toflu
                QString serviceName;
                std::multiset<std::pair< CScript, std::tuple<std::string, std::string, std::string>>> services;
                ServiceList.GetServiceAddresses(services);
                for(std::multiset< std::pair< CScript, std::tuple<std::string, std::string, std::string> > >::const_iterator it = services.begin(); it!=services.end(); it++ )
                {
                    if (service == QString::fromStdString(get<1>(it->second))) {
                        serviceName = QString::fromStdString(get<0>(it->second));
                    }
                }

                cachedTicketTable.insert(lowerIndex, TicketTableEntry(name, location, datetime, price, address, serviceName));
                parent->endInsertRows();
                break;
            }
            case CT_UPDATED:
            {
                if(!inModel)
                {
                    break;
                }
                lower->name = name;
                lower->location = location;
                lower->datetime = datetime;
                lower->price = price;
                lower->address = address;
                lower->service = service;
                parent->emitDataChanged(lowerIndex);
                break;
            }
            case CT_DELETED:
            {
                if(!inModel)
                {
                    break;
                }
                parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                cachedTicketTable.erase(lower, upper);
                parent->endRemoveRows();
                break;
            }
        }
    }

    int size()
    {
        return cachedTicketTable.size();
    }

    TicketTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedTicketTable.size())
        {
            return &cachedTicketTable[idx];
        }
        else
        {
            return 0;
        }
    }

};

TicketTableModel::TicketTableModel(CWallet *wallet, WalletModel *parent) :
        QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Name") << tr("Location") << tr("Date and time") << tr("Price") << tr("Address") << tr("Service");
    priv = new TicketTablePriv(wallet, this);
    priv->refreshTicketTable();
}

TicketTableModel::~TicketTableModel()
{
    delete priv;
}


int TicketTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int TicketTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant TicketTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    TicketTableEntry *rec = static_cast<TicketTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
            case Name:
                return rec->name;
            case Location:
                return rec->location;
            case DateTime:
                return rec->datetime;
            case Price:
                return rec->price;
            case Address:
                return rec->address;
            case Service:
                return rec->service;
        }
    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        if(index.column() == Address)
        {
            font = GUIUtil::bitcoinAddressFont();
        }
        return font;
    }
    return QVariant();
}

QVariant TicketTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags TicketTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    TicketTableEntry *rec = static_cast<TicketTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    // Can edit address and label for sending addresses,
    // and only label for receiving addresses.
    return retval;
}

QModelIndex TicketTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TicketTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void TicketTableModel::updateTicketEntry(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service, int status)
{
    // Update ticket page model from Bitcoin core
    priv->updateTicketEntry(name, location, datetime, price, address, service, status);
}

QString TicketTableModel::addRow(const QString &name, const QString &location,  const QString &datetime, const QString &price, const QString &address, const QString &service)
{
    std::string strName = name.toStdString();
    std::string strLocation = location.toStdString();
    std::string strDateTime = datetime.toStdString();
    std::string strPrice = price.toStdString();
    std::string strAddress = address.toStdString();
    std::string strService = service.toStdString();

    //editStatus = OK;

    /*if(type == Send)
    {
        if(!walletModel->validateAddress(address))
        {
            editStatus = INVALID_ADDRESS;
            return QString();
        }
        // Check for duplicate addresses
        {
            LOCK(wallet->cs_wallet);
            if(wallet->mapAddressBook.count(CBitcoinAddress(strAddress).Get()))
            {
                editStatus = DUPLICATE_ADDRESS;
                return QString();
            }
        }
    }
    else if(type == Receive)
    {
        // Generate a new address to associate with given label
        CPubKey newKey;
        if(!wallet->GetKeyFromPool(newKey))
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if(!ctx.isValid())
            {
                // Unlock wallet failed or was cancelled
                editStatus = WALLET_UNLOCK_FAILURE;
                return QString();
            }
            if(!wallet->GetKeyFromPool(newKey))
            {
                editStatus = KEY_GENERATION_FAILURE;
                return QString();
            }
        }
        strAddress = CBitcoinAddress(newKey.GetID()).ToString();
    }
    else
    {
        return QString();
    }*/

    // Add entry
    /*{
        LOCK(wallet->cs_wallet);
        wallet->SetAddressBook(CBitcoinAddress(strAddress).Get(), strLabel,
                               (type == Send ? "send" : "receive"));
    }*/
    return QString::fromStdString(strAddress);
}

void TicketTableModel::emitDataChanged(int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
