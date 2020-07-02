//
// Created by Lenovo on 6/22/2020.
//

#ifndef SMILEYCOIN_SERVICETABLEMODEL_H
#define SMILEYCOIN_SERVICETABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class ServiceTablePriv;
class WalletModel;

class CWallet;


class ServiceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ServiceTableModel(bool viewAll, CWallet *wallet, WalletModel *parent = 0);
    ~ServiceTableModel();

    enum ColumnIndex {
        Name = 0,   /**< User specified label */
        Address = 1,  /**< Bitcoin address */
        Type = 2
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole /**< Type of address (#Send or #Receive) */
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QString addRow(const QString &name, const QString &address, const QString &type);

private:
    WalletModel *walletModel;
    CWallet *wallet;
    ServiceTablePriv *priv;
    QStringList columns;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public slots:
    /* Update service list from core.*/
    void updateEntry(const QString &name, const QString &address, const QString &type, int status);

friend class ServiceTablePriv;

};

#endif //SMILEYCOIN_SERVICETABLEMODEL_H
