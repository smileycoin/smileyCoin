//
// Created by Lenovo on 6/22/2020.
//

#include "servicetablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet.h"
#include "servicelistdb.h"

#include <QFont>

struct ServiceTableEntry
{
    QString name;
    QString address;
    QString type;

    ServiceTableEntry() {}
    ServiceTableEntry(const QString &name, const QString &address, const QString &type):
    name(name), address(address), type(type) {}
};

struct ServiceTableEntryLessThan
{
    bool operator()(const ServiceTableEntry &a, const ServiceTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const ServiceTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const ServiceTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class ServiceTablePriv
{
public:
    CWallet *wallet;
    QList<ServiceTableEntry> cachedServiceTable;
    ServiceTableModel *parent;


    ServiceTablePriv(bool viewAll, CWallet *wallet, ServiceTableModel *parent):
        viewAll(viewAll), wallet(wallet), parent(parent) {}

    void refreshServiceTable()
    {
        cachedServiceTable.clear();
        {
            LOCK(wallet->cs_wallet);
            std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> retset;

            if (viewAll) {
                ServiceList.GetServiceAddresses(retset);
                for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator
                it = retset.begin(); it!=retset.end(); it++ )
                {
                    cachedServiceTable.append(ServiceTableEntry(QString::fromStdString(get<1>(it->second)),
                            QString::fromStdString(it->first), QString::fromStdString(get<2>(it->second))));
                }
            } else {
                ServiceList.GetMyServiceAddresses(retset);
                for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator
                it = retset.begin(); it!=retset.end(); it++ )
                {
                    cachedServiceTable.append(ServiceTableEntry(QString::fromStdString(get<1>(it->second)),
                            QString::fromStdString(it->first), QString::fromStdString(get<2>(it->second))));
                }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        //qSort(cachedServiceTable.begin(), cachedServiceTable.end(), ServiceTableEntryLessThan());
    }

    void updateEntry(const QString &name, const QString &address, const QString &type, int status)
    {
        // Find name in model
        QList<ServiceTableEntry>::iterator lower = std::lower_bound(
                cachedServiceTable.begin(), cachedServiceTable.end(), address, ServiceTableEntryLessThan());
        QList<ServiceTableEntry>::iterator upper = std::upper_bound(
                cachedServiceTable.begin(), cachedServiceTable.end(), address, ServiceTableEntryLessThan());
        int lowerIndex = (lower - cachedServiceTable.begin());
        int upperIndex = (upper - cachedServiceTable.begin());
        bool inModel = (lower != upper);
        //ServiceTableEntry::Type newEntryType = translateTransactionType(purpose, isMine);

        switch(status)
        {
            case CT_NEW:
                if(inModel)
                {
                    error("ServiceTablePriv::updateEntry : Warning: Got CT_NEW, but entry is already in model");
                    break;
                }
                parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
                cachedServiceTable.insert(lowerIndex, ServiceTableEntry(name, address, type));
                parent->endInsertRows();
                break;
            case CT_UPDATED:
                if(!inModel)
                {
                    error("ServiceTablePriv::updateEntry : Warning: Got CT_UPDATED, but entry is not in model");
                    break;
                }
                lower->name = name;
                lower->address = address;
                lower->type = type;
                parent->emitDataChanged(lowerIndex);
                break;
            case CT_DELETED:
                if(!inModel)
                {
                    error("ServiceTablePriv::updateEntry : Warning: Got CT_DELETED, but entry is not in model");
                    break;
                }
                parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                cachedServiceTable.erase(lower, upper);
                parent->endRemoveRows();
                break;
        }
    }

    int size()
    {
        return cachedServiceTable.size();
    }

    ServiceTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedServiceTable.size())
        {
            return &cachedServiceTable[idx];
        }
        else
        {
            return 0;
        }
    }

private:
    bool viewAll;

};

ServiceTableModel::ServiceTableModel(bool viewAll, CWallet *wallet, WalletModel *parent) :
        QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Name") << tr("Address") << tr("Type");
    priv = new ServiceTablePriv(viewAll, wallet, this);
    priv->refreshServiceTable();
}

ServiceTableModel::~ServiceTableModel()
{
    delete priv;
}


int ServiceTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int ServiceTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ServiceTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    ServiceTableEntry *rec = static_cast<ServiceTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
            case Name:
                return rec->name;
            case Address:
                return rec->address;
            case Type:
                return rec->type;
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

QVariant ServiceTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags ServiceTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    ServiceTableEntry *rec = static_cast<ServiceTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    // Can edit address and label for sending addresses,
    // and only label for receiving addresses.
    return retval;
}

QModelIndex ServiceTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    ServiceTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void ServiceTableModel::updateEntry(const QString &name,
                                    const QString &address, const QString &type, int status)
{
    // Update service page model from Bitcoin core
    priv->updateEntry(name, address, type, status);
}

QString ServiceTableModel::addRow(const QString &name, const QString &address, const QString &type)
{
    std::string strName = name.toStdString();
    std::string strAddress = address.toStdString();
    std::string strType = type.toStdString();

    return QString::fromStdString(strAddress);
}

void ServiceTableModel::emitDataChanged(int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}