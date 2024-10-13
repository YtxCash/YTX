#ifndef TABLEWIDGETORDER_H
#define TABLEWIDGETORDER_H

#include <QComboBox>

#include "component/settings.h"
#include "table/model/tablemodel.h"
#include "tree/model/treemodel.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetOrder;
}

class TableWidgetOrder final : public TableWidget {
    Q_OBJECT

public:
    TableWidgetOrder(NodeShadow* node_shadow, SPSqlite sql, TableModel* order_table, TreeModel* stakeholder_tree, CSettings& settings, int party_unit,
        QWidget* parent = nullptr);
    ~TableWidgetOrder();

signals:
    void SUpdateLocked(int node_id, bool checked);

public slots:
    void RUpdateStakeholder();
    void RUpdateLocked(int node_id, bool checked);

    void RUpdateLeafValueOne(int node_id, double diff); // first
    void RUpdateLeafValueOrder(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff);

public:
    TableModel* Model() override { return order_table_; }
    QTableView* View() override;

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

private:
    void IniDialog();
    void IniData();
    void IniCombo(QComboBox* combo, int mark);
    void LockWidgets(bool locked, bool branch);
    void IniUnit(int unit);

private:
    Ui::TableWidgetOrder* ui;
    NodeShadow* node_shadow_ {};
    SPSqlite sql_ {};
    int party_unit_ {};
    TableModel* order_table_ {};
    TreeModel* stakeholder_tree_ {};
    CSettings& settings_;

    const QString info_node_ {};
    const int node_id_ {};
};

#endif // TABLEWIDGETORDER_H
