#ifndef INSERTNODEORDER_H
#define INSERTNODEORDER_H

#include <QComboBox>
#include <QDialog>

#include "component/data.h"
#include "component/info.h"
#include "component/settings.h"
#include "component/using.h"

namespace Ui {
class InsertNodeOrder;
}

class InsertNodeOrder final : public QDialog {
    Q_OBJECT

public:
    InsertNodeOrder(Node* node, const SectionRule* section_rule, Tree* stakeholder, CStringHash* product_leaf, const Info* info, QWidget* parent = nullptr);
    ~InsertNodeOrder();

private slots:
    void on_chkBoxBranch_toggled(bool checked);
    void on_comboCompany_currentTextChanged(const QString& arg1);
    void on_comboCompany_currentIndexChanged(int index);
    void on_chkBoxRefund_toggled(bool checked);
    void on_comboEmployee_currentIndexChanged(int index);
    void on_rBtnCash_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);
    void on_rBtnPending_toggled(bool checked);

    void on_pBtnInsertStakeholder_clicked();

    void on_dateEdit_dateChanged(const QDate& date);

private:
    void IniDialog();
    void IniCombo(QComboBox* combo);
    void IniCombo(QComboBox* combo, const NodeHash* node_hash, CStringHash* path_hash, int mark);
    void IniConnect();
    void SetData();
    void ZeroSettlement();

private:
    Ui::InsertNodeOrder* ui;

    Node* node_ {};
    const NodeHash* stakeholder_node_hash_ {};
    CStringHash* stakeholder_branch_ {};
    CStringHash* product_leaf_ {};
    const Info* info_ {};
    const SectionRule* section_rule_ {};
    Tree* stakeholder_ {};
};

#endif // INSERTNODEORDER_H
