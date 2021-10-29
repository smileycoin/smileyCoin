//
// Created by Lenovo on 6/30/2020.
//

#ifndef SMILEYCOIN_COUPONPAGE_H
#define SMILEYCOIN_COUPONPAGE_H

#include <QDialog>

#include "walletmodel.h"
#include "guiutil.h"

#include <QDialog>
#include <QWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDateTimeEdit>
#include <QDateTime>
#include <QTableView>
#include <QScrollBar>
#include <QTableWidget>
#include <QDesktopWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QApplication>
#include <QMessageBox>
#include <QTextTableFormat>
#include <QRadioButton>
#include <QStyledItemDelegate>


class WalletModel;
class OptionsModel;
class SendCoinsDialog;
class QValidatedLineEdit;
class CouponTableModel;

namespace Ui {
    class CouponPage;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
class QMenu;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
QT_END_NAMESPACE

/** Widget that shows a list of sending or receiving addresses.
  */
class CouponPage : public QDialog
{
    Q_OBJECT

public:
    explicit CouponPage(QWidget *parent);
    ~CouponPage();

    void setWalletModel(WalletModel *walletModel);
    void setCouponModel(CouponTableModel *couponModel);

public slots:
    void done(int retval);
    void chooseService(QString text);

private:
    Ui::CouponPage *ui;
    CouponTableModel *couponModel;
    WalletModel *walletModel;
    QString returnValue;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> allServices;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string>>> myCoupons;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> coupons;
    QSortFilterProxyModel *proxyModel;
    QVBoxLayout *verticalLayout;
    QLineEdit *addressWidget;
    QComboBox *typeWidget;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onNewCouponAction();
    void onDeleteCouponAction();
    void onBuyCouponAction();

signals:
    void doubleClicked(const QModelIndex&);

};


#endif //SMILEYCOIN_COUPONPAGE_H
