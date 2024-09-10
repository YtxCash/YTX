#ifndef EDITNODEORDER_H
#define EDITNODEORDER_H

#include <QComboBox>
#include <QDialog>

#include "component/data.h"
#include "component/info.h"
#include "component/settings.h"

namespace Ui {
class EditNodeOrder;
}

class EditNodeOrder final : public QDialog {
    Q_OBJECT

public:
    EditNodeOrder(Node* node, CSectionRule& section_rule, AbstractTreeModel* order_model, Tree* stakeholder, const AbstractTreeModel& product_model,
        CInfo& info, QWidget* parent = nullptr);
    ~EditNodeOrder();

public slots:
    void accept() override;
    void reject() override;
    void RUpdatePartyEmployee();

private slots:
    void on_chkBoxBranch_toggled(bool checked);
    void on_comboParty_currentTextChanged(const QString& arg1);
    void on_comboParty_currentIndexChanged(int index);
    void on_chkBoxRefund_toggled(bool checked);
    void on_comboEmployee_currentIndexChanged(int index);
    void on_rBtnCash_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);
    void on_rBtnPending_toggled(bool checked);
    void on_pBtnInsertParty_clicked();
    void on_dateEdit_dateChanged(const QDate& date);
    void on_pBtnPostOrder_toggled(bool checked);

    void on_dSpinDiscount_editingFinished();

private:
    void IniDialog();
    void IniCombo(QComboBox* combo);
    void IniCombo(QComboBox* combo, int mark);
    void IniConnect();
    void SetData();
    void SetWidgetsEnabled(bool enabled);
    void SetWidgetsEnabledBranch(bool enabled);
    void SetWidgetsEnabledPost(bool enabled);
    void ZeroSettlement();
    void EnabledPost(bool enabled);

private:
    Ui::EditNodeOrder* ui;

    Node* node_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};
    Tree* stakeholder_ {};
    AbstractTreeModel* order_model_ {};
    const AbstractTreeModel& product_model_;
    bool saved_ {};
};

#endif // EDITNODEORDER_H
