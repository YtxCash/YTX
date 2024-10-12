#ifndef INSERTNODEORDER_H
#define INSERTNODEORDER_H

#include <QComboBox>
#include <QDialog>

#include "component/settings.h"
#include "tree/model/treemodel.h"

namespace Ui {
class InsertNodeOrder;
}

class InsertNodeOrder final : public QDialog {
    Q_OBJECT

public:
    InsertNodeOrder(NodeShadow* node_shadow, SPSqlite sql, TableModel* order_table, TreeModel* stakeholder_model, CSettings& settings, int unit_party,
        QWidget* parent = nullptr);
    ~InsertNodeOrder();

signals:
    void SUpdateNodeID(int node_id);
    void SUpdateLocked(int node_id, bool checked);

public slots:
    void accept() override;
    void RUpdateStakeholder();
    void RUpdateLocked(int node_id, bool checked);
    void RUpdateLeafValueOne(int node_id, double diff); // first
    void RUpdateLeafValueOrder(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff);

public:
    QTableView* View();

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

    void on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1);

private:
    void IniDialog();
    void IniCombo(QComboBox* combo, int mark);
    void IniConnect();
    void LockWidgets(bool locked, bool branch);
    void IniUnit(int unit);

private:
    Ui::InsertNodeOrder* ui;

    NodeShadow* node_shadow_ {};
    SPSqlite sql_ {};
    int unit_party_ {};
    TableModel* order_table_ {};
    TreeModel* stakeholder_tree_ {};
    CSettings& settings_;

    const QString info_node_ {};
    int node_id_ {};
};

#endif // INSERTNODEORDER_H
