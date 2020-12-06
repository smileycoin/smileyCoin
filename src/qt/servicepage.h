//
// Created by Lenovo on 4/28/2020.
//

#ifndef SERVICEPAGE_H
#define SERVICEPAGE_H

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
#include <QTabWidget>
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


class WalletModel;
class OptionsModel;
class SendCoinsDialog;
class QValidatedLineEdit;
class ServiceTableModel;

namespace Ui {
    class ServicePage;
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
class ServicePage : public QDialog
{
    Q_OBJECT

public:
    explicit ServicePage(QWidget *parent);
    ~ServicePage();

    void setWalletModel(WalletModel *walletModel);
    void setServiceModel(ServiceTableModel *serviceModel);

public slots:
    void done(int retval);

private:
    Ui::ServicePage *ui;
    ServiceTableModel *serviceModel;
    WalletModel *walletModel;
    QString returnValue;
    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    QSortFilterProxyModel *proxyModel;
    QVBoxLayout *verticalLayout;
    QLineEdit *addressWidget;
    QComboBox *typeWidget;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onNewServiceAction();
    void onDeleteServiceAction();
    void onViewAllServices();
    void onViewMyServices();
    void showServiceDetails();

    signals:
        void doubleClicked(const QModelIndex&);

};

#endif //SERVICEPAGE_H
