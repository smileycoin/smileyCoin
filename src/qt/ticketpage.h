//
// Created by Lenovo on 6/30/2020.
//

#ifndef SMILEYCOIN_TICKETPAGE_H
#define SMILEYCOIN_TICKETPAGE_H

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
class TicketTableModel;

namespace Ui {
    class TicketPage;
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
class TicketPage : public QDialog
{
    Q_OBJECT

public:
    explicit TicketPage(QWidget *parent);
    ~TicketPage();

    void setWalletModel(WalletModel *walletModel);
    void setTicketModel(TicketTableModel *ticketModel);

public slots:
    void done(int retval);
    void chooseService(QString text);

private:
    Ui::TicketPage *ui;
    TicketTableModel *ticketModel;
    WalletModel *walletModel;
    QString returnValue;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> allServices;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string>>> myTickets;
    QSortFilterProxyModel *proxyModel;
    QVBoxLayout *verticalLayout;
    QLineEdit *addressWidget;
    QComboBox *typeWidget;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onNewTicketAction();
    void onDeleteTicketAction();

signals:
    void doubleClicked(const QModelIndex&);

};


#endif //SMILEYCOIN_TICKETPAGE_H
