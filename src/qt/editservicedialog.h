//
// Created by Lenovo on 6/15/2020.
//

#ifndef SMILEYCOIN_EDITSERVICEDIALOG_H
#define SMILEYCOIN_EDITSERVICEDIALOG_H

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
#include <QDebug>
#include <QStringList>

class WalletModel;
class OptionsModel;
class SendCoinsDialog;
class QValidatedLineEdit;

namespace Ui {
    class EditServiceDialog;
}

/** Dialog for creating a new service.
 */
class EditServiceDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewService,
        NewTicket
    };

    explicit EditServiceDialog(Mode mode, QWidget *parent);
    ~EditServiceDialog();

    void setModel(WalletModel *model);

    QString getAddress() const;
    void setAddress(const QString &address);

    //void dialogIsFinished(int);

public slots:
    void accept();

private:
    Ui::EditServiceDialog *ui;
    Mode mode;
    WalletModel *model;
    //QMessageBox *mgx;

    QString address;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

//private slots:
  //  void buttonBoxClicked(QAbstractButton*);
};

#endif //SMILEYCOIN_EDITSERVICEDIALOG_H
