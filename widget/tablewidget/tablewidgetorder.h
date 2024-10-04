#ifndef TABLEWIDGETORDER_H
#define TABLEWIDGETORDER_H

#include <QComboBox>

#include "table/model/tablemodel.h"
#include "tree/model/treemodel.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetOrder;
}

class TableWidgetOrder final : public TableWidget {
    Q_OBJECT

public:
    TableWidgetOrder(Node* node, TreeModel* order_model, TreeModel* stakeholder_model, const TreeModel* product_model, int value_decimal, int unit_party,
        QWidget* parent = nullptr);
    ~TableWidgetOrder();

    void SetModel(TableModel* model) override;

    TableModel* Model() override { return order_table_model_; }
    TableView* View() override;

public slots:
    void RAccept();
    void RReject();
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
    void on_spinFirst_editingFinished();
    void on_dSpinSecond_editingFinished();
    void on_dSpinDiscount_editingFinished();
    void on_dSpinInitialTotal_editingFinished();

    void on_lineDescription_textChanged(const QString& arg1);
    void on_spinFirst_valueChanged(int arg1);
    void on_dSpinSecond_valueChanged(double arg1);
    void on_dSpinDiscount_valueChanged(double arg1);
    void on_dSpinInitialTotal_valueChanged(double arg1);

private:
    void IniDialog();
    void IniData();
    void IniCombo(QComboBox* combo, int mark);
    void LockWidgets(bool locked, bool branch);
    void UpdateUnit(int unit);

private:
    Ui::TableWidgetOrder* ui;
    TableModel* order_table_model_ {};

    Node* node_ {};
    int unit_party_ {};
    int value_decimal_ {};
    TreeModel* stakeholder_model_ {};
    TreeModel* order_model_ {};
    const TreeModel* product_model_ {};
};

#endif // TABLEWIDGETORDER_H
