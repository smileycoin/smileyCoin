//
// Created by Lenovo on 7/3/2020.
//

#ifndef SMILEYCOIN_TICKETTABLEMODEL_H
#define SMILEYCOIN_TICKETTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class TicketTablePriv;
class WalletModel;

class CWallet;


class TicketTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TicketTableModel(CWallet *wallet, WalletModel *parent = 0);
    ~TicketTableModel();

    enum ColumnIndex {
        Name = 0,   /**< User specified label */
        Location = 1,  /**< Bitcoin address */
        DateTime = 2,
        Price = 3,
        Address = 4,
        Service = 5
    };

    /*enum RoleIndex {
        TypeRole = Qt::UserRole
    };*/

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QString addRow(const QString &name, const QString &location, const QString &datetime, const QString &price, const QString &address, const QString &service);

private:
    WalletModel *walletModel;
    CWallet *wallet;
    TicketTablePriv *priv;
    QStringList columns;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public slots:
        /* Update ticket list from core.*/
        void updateTicketEntry(const QString &name, const QString &location, const QString &datetime,
                const QString &price, const QString &address, const QString &service, int status);

    friend class TicketTablePriv;

};

#endif //SMILEYCOIN_TICKETTABLEMODEL_H
