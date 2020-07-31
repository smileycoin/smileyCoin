#ifndef TRACEABILITYPAGE_H
#define TRACEABILITYPAGE_H

#include "walletmodel.h"
#include "transactionview.h"
#include "transactiontablemodel.h"

#include <QWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class WalletModel;
class OptionsModel;
class SendCoinsDialog;
class QValidatedLineEdit;

/** Widget that shows a list of sending or receiving addresses.
  */
class TraceabilityPage : public QWidget
{
    Q_OBJECT

public:
    enum Mode {
        //ForChoosingPreviousTransaction,
        ForCreatingTransaction,
    };

    explicit TraceabilityPage(Mode mode, QWidget *parent);

    void setModel(WalletModel *model);

public slots:
    void clear();
    void accept();

private:
    WalletModel *model;
    Mode mode;

    SendCoinsDialog *sendCoinsDialog;

    QGroupBox *formGroupBox;

    QComboBox *transactionTypeCombo;
    QLabel *typeOfTransaction;
    QValidatedLineEdit *traceabilityPreviousInput;
    QLabel *traceabilityInformation;
    QValidatedLineEdit *traceabilityAddressInput;
    QValidatedLineEdit *traceabilityNumberInput;
    QValidatedLineEdit *traceabilityAmountInput;
    QValidatedLineEdit *traceabilityLocationInput;
    QPushButton *sendButton;

    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

private slots:
    void onTraceabilityAction();
    void onTransactionTypeChanged(const QString &text);
};

#endif //TRACEABILITYPAGE_H
