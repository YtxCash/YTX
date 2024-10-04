#ifndef INSERTNODEORDER_H
#define INSERTNODEORDER_H

#include <QComboBox>
#include <QDialog>

#include "tree/model/treemodel.h"
#include "widget/tablewidget/tableview.h"

namespace Ui {
class InsertNodeOrder;
}

class InsertNodeOrder final : public QDialog {
    Q_OBJECT

public:
    InsertNodeOrder(
        Node* node, SPSqlite sql, TableModel* order_table, TreeModel* stakeholder_model, int value_decimal, int unit_party, QWidget* parent = nullptr);
    ~InsertNodeOrder();

signals:
    void SUpdateLocked(int node_id, bool checked);

public slots:
    void accept() override;
    void reject() override;
    void RUpdateStakeholder();
    void RUpdateLocked(int node_id, bool checked);

public:
    TableView* View();

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

    void on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1);

    void on_lineDescription_editingFinished();
    void on_dSpinFirst_editingFinished();
    void on_dSpinSecond_editingFinished();
    void on_dSpinDiscount_editingFinished();
    void on_dSpinInitialTotal_editingFinished();

private:
    void IniDialog();
    void IniCombo(QComboBox* combo, int mark);
    void IniConnect();
    void LockWidgets(bool locked, bool branch);
    void UpdateUnit(int unit);

private:
    Ui::InsertNodeOrder* ui;

    SPSqlite sql_ {};

    Node* node_ {};
    int unit_party_ {};
    int value_decimal_ {};
    TableModel* order_table_ {};
    TreeModel* stakeholder_model_ {};

    const QString info_node_ {};

    int node_id_ {};
    bool is_saved_ { false };
};

#endif // INSERTNODEORDER_H
