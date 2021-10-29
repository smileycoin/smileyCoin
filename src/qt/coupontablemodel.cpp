//
// Created by Lenovo on 7/3/2020.
//

#include "coupontablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet.h"
#include "servicelistdb.h"
#include "serviceitemlistdb.h"


#include <QFont>

struct CouponTableEntry
{
    QString name;
    QString location;
    QString datetime;
    QString price;
    QString address;
    QString service;

    CouponTableEntry() {}
    CouponTableEntry(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service):
            name(name), location(location), datetime(datetime), price(price), address(address), service(service) {}
};

struct CouponTableEntryLessThan
{
    bool operator()(const CouponTableEntry &a, const CouponTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const CouponTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const CouponTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class CouponTablePriv
{
public:
    CWallet *wallet;
    QList<CouponTableEntry> cachedCouponTable;
    CouponTableModel *parent;
    std::string serviceFilter;


    CouponTablePriv(std::string serviceFilter, CWallet *wallet, CouponTableModel *parent):
            serviceFilter(serviceFilter), wallet(wallet), parent(parent) {}

    void refreshCouponTable()
    {
        cachedCouponTable.clear();
        {
            LOCK(wallet->cs_wallet);

            QString serviceName;
            std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> services;
            ServiceList.GetServiceAddresses(services);

            std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> coupons;
            ServiceItemList.GetCouponList(coupons);
            for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > >::const_iterator
                        t = coupons.begin(); t!=coupons.end(); t++ )
            {
                for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator s = services.begin(); s!=services.end(); s++ )
                {
                    // Display corresponding service name instead of address
                    if (QString::fromStdString(get<1>(t->second)) == QString::fromStdString(s->first) && is_before(get<4>(t->second))) {
                        if (serviceFilter == "All") {
                            cachedCouponTable.append(CouponTableEntry(QString::fromStdString(get<3>(t->second)),
                                    QString::fromStdString(get<2>(t->second)),
                                    QString::fromStdString(get<4>(t->second)),
                                    QString::fromStdString(get<5>(t->second)),
                                    QString::fromStdString(t->first),
                                    QString::fromStdString(get<1>(s->second))));
                        } else if (serviceFilter == get<1>(s->second)) { // If dropdown selection matches service name
                            cachedCouponTable.append(CouponTableEntry(QString::fromStdString(get<3>(t->second)),
                                    QString::fromStdString(get<2>(t->second)),
                                    QString::fromStdString(get<4>(t->second)),
                                    QString::fromStdString(get<5>(t->second)),
                                    QString::fromStdString(t->first),
                                    QString::fromStdString(get<1>(s->second))));
                        }
                    }
                }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        //qSort(cachedCouponTable.begin(), cachedCouponTable.end(), CouponTableEntryLessThan());
    }

    void updateCouponEntry(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service, int status)
    {
        // Find address / label in model
        QList<CouponTableEntry>::iterator lower = std::lower_bound(
                cachedCouponTable.begin(), cachedCouponTable.end(), address, CouponTableEntryLessThan());
        QList<CouponTableEntry>::iterator upper = std::upper_bound(
                cachedCouponTable.begin(), cachedCouponTable.end(), address, CouponTableEntryLessThan());
        int lowerIndex = (lower - cachedCouponTable.begin());
        int upperIndex = (upper - cachedCouponTable.begin());
        bool inModel = (lower != upper);
        //ServiceTableEntry::Type newEntryType = translateTransactionType(purpose, isMine);

        switch(status)
        {
            case CT_NEW:
            {
                if(inModel)
                {
                    error("CouponTablePriv::updateEntry : Warning: Got CT_NEW, but entry is already in model");
                    break;
                }
                parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
                cachedCouponTable.insert(lowerIndex, CouponTableEntry(name, location, datetime, price, address, service));
                parent->endInsertRows();
                break;
            }
            case CT_UPDATED:
            {
                if(!inModel)
                {
                    error("CouponTablePriv::updateEntry : Warning: Got CT_UPDATED, but entry is not in model");
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
                    error("CouponTablePriv::updateEntry : Warning: Got CT_DELETED, but entry is not in model");
                    break;
                }
                parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                cachedCouponTable.erase(lower, upper);
                parent->endRemoveRows();
                break;
            }
        }
    }

    int size()
    {
        return cachedCouponTable.size();
    }

    CouponTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedCouponTable.size())
        {
            return &cachedCouponTable[idx];
        }
        else
        {
            return 0;
        }
    }

};

CouponTableModel::CouponTableModel(std::string serviceFilter, CWallet *wallet, WalletModel *parent) :
        QAbstractTableModel(parent),walletModel(parent),wallet(wallet), serviceFilter(serviceFilter), priv(0)
{
    columns << tr("Name") << tr("Location") << tr("Date and time") << tr("Price") << tr("Address") << tr("Service");
    priv = new CouponTablePriv(serviceFilter, wallet, this);
    priv->refreshCouponTable();
}

CouponTableModel::~CouponTableModel()
{
    delete priv;
}


int CouponTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int CouponTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant CouponTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    CouponTableEntry *rec = static_cast<CouponTableEntry*>(index.internalPointer());

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

QVariant CouponTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags CouponTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    CouponTableEntry *rec = static_cast<CouponTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    // Can edit address and label for sending addresses,
    // and only label for receiving addresses.
    return retval;
}

QModelIndex CouponTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CouponTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void CouponTableModel::updateCouponEntry(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service, int status)
{
    // Update coupon page model from Bitcoin core
    priv->updateCouponEntry(name, location, datetime, price, address, service, status);
}

QString CouponTableModel::addRow(const QString &name, const QString &location,  const QString &datetime, const QString &price, const QString &address, const QString &service)
{
    std::string strName = name.toStdString();
    std::string strLocation = location.toStdString();
    std::string strDateTime = datetime.toStdString();
    std::string strPrice = price.toStdString();
    std::string strAddress = address.toStdString();
    std::string strService = service.toStdString();
    return QString::fromStdString(strAddress);
}

void CouponTableModel::emitDataChanged(int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
