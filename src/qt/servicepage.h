//
// Created by Lenovo on 4/28/2020.
//

#ifndef SERVICEPAGE_H
#define SERVICEPAGE_H

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

/** Widget that shows a list of sending or receiving addresses.
  */
class ServicePage : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        ForConfirmingService,
        AllServices,
        MyServices
    };

    explicit ServicePage(QWidget *parent = 0);
    //explicit ServicePage(Mode mode, std::vector<std::tuple<std::string, std::string, std::string>> serviceObject, QWidget *parent);

    void setModel(WalletModel *model);

    enum ServiceType {
        All,
        TicketSales,
        UBI,
        BookChapter,
        Traceability,
        NonprofitOrganization
    };
    enum ColumnWidths {
        STATUS_COLUMN_WIDTH = 23,
        DATE_COLUMN_WIDTH = 120,
        TYPE_COLUMN_WIDTH = 120,
        DATA_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 120,
        MINIMUM_COLUMN_WIDTH = 23
    };


private:
    WalletModel *model;
    Mode mode;
    std::vector<std::tuple<std::string, std::string, std::string>> allServices;
    std::vector<std::tuple<std::string, std::string, std::string>> myServices;
    std::vector<std::tuple<std::string, std::string, std::string>> serviceObject;

    SendCoinsDialog *sendCoinsDialog;

    QGroupBox *formGroupBox;

    QValidatedLineEdit *serviceNameInput;
    QValidatedLineEdit *serviceAddressInput;
    QComboBox *serviceTypeCombo;
    QPushButton *serviceButton;

    QValidatedLineEdit *ticketNameInput;
    QValidatedLineEdit *ticketLocInput;
    QValidatedLineEdit *ticketPriceInput;
    QValidatedLineEdit *ticketAddressInput;
    QDateTimeEdit *dateTimeEdit;
    QDateTime *currDateTime;
    QPushButton *ticketButton;

    QVBoxLayout *verticalLayout;
    QLabel *labelExplanation;
    QTableView *serviceView;
    QTableWidget *serviceTableWidget;
    QTableWidget *table;
    QLineEdit *nameWidget;
    QLineEdit *addressWidget;
    QComboBox *typeWidget;
    QRadioButton *viewAllServices;
    QRadioButton *viewMyServices;

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onServiceAction();
    void onTicketAction();
    void onNewServiceAction();
    void onViewAllServices();
    void onViewMyServices();

    signals:
        inline void cellSelected2(int nRow, int nCol);


public slots:
    void clear();
    void accept();

};

#endif //SERVICEPAGE_H
