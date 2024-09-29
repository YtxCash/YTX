#ifndef EDITNODEORDER_H
#define EDITNODEORDER_H

#include <QComboBox>
#include <QDialog>

#include "tree/model/treemodel.h"

namespace Ui {
class EditNodeOrder;
}

class EditNodeOrder final : public QDialog {
    Q_OBJECT

public:
    EditNodeOrder(Node* node, TreeModel* order_model, TreeModel* stakeholder_model, const TreeModel* product_model, int value_decimal, int unit_party,
        QWidget* parent = nullptr);
    ~EditNodeOrder();

public slots:
    void accept() override;
    void reject() override;
    void RUpdateStakeholder();
    void RUpdateOrder(const QVariant& value, TreeEnumOrder column);

private slots:
    void on_comboParty_editTextChanged(const QString& arg1);

    void on_comboParty_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_chkBoxRefund_toggled(bool checked);
    void on_pBtnLockOrder_toggled(bool checked);
    void on_rBtnCash_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);
    void on_rBtnPending_toggled(bool checked);

    void on_pBtnInsertParty_clicked();

    void on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time);

    void on_lineDescription_editingFinished();
    void on_dSpinFirst_editingFinished();
    void on_dSpinSecond_editingFinished();
    void on_dSpinDiscount_editingFinished();
    void on_dSpinInitialTotal_editingFinished();

    void on_lineDescription_textChanged(const QString& arg1);
    void on_dSpinFirst_valueChanged(double arg1);
    void on_dSpinSecond_valueChanged(double arg1);
    void on_dSpinDiscount_valueChanged(double arg1);
    void on_dSpinInitialTotal_valueChanged(double arg1);

private:
    void IniDialog();
    void IniData();
    void IniCombo(QComboBox* combo, int mark);
    void IniConnect();
    void LockWidgets(bool locked, bool branch);
    void UpdateUnit(int unit);

private:
    Ui::EditNodeOrder* ui;

    Node* node_ {};
    int unit_party_ {};
    int value_decimal_ {};
    TreeModel* stakeholder_model_ {};
    TreeModel* order_model_ {};
    const TreeModel* product_model_ {};
};

#endif // EDITNODEORDER_H
