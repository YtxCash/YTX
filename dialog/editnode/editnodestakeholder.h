#ifndef EDITNODESTAKEHOLDER_H
#define EDITNODESTAKEHOLDER_H

#include <QComboBox>
#include <QDialog>

#include "component/using.h"
#include "tree/model/treemodel.h"
#include "tree/node.h"

namespace Ui {
class EditNodeStakeholder;
}

class EditNodeStakeholder final : public QDialog {
    Q_OBJECT

public:
    EditNodeStakeholder(Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool branch_enable, bool unit_enable,
        int amount_decimal, TreeModel* stakeholder_tree, QWidget* parent = nullptr);
    ~EditNodeStakeholder();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_spinDeadline_editingFinished();
    void on_dSpinPaymentPeriod_editingFinished();
    void on_dSpinTaxRate_editingFinished();

    void on_chkBoxBranch_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);

    void on_comboUnit_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_plainTextEdit_textChanged();

private:
    void IniDialog(CStringHash& unit_hash, TreeModel* stakeholder_tree, int common_decimal);
    void IniComboWithStringHash(QComboBox* combo, CStringHash& hash);
    void IniComboEmployee(TreeModel* stakeholder_tree);
    void IniConnect();
    void Data(Node* node, bool branch_enable, bool unit_enable);

private:
    Ui::EditNodeStakeholder* ui;
    Node* node_ {};

    QString parent_path_ {};
    CStringList& name_list_ {};
};

#endif // EDITNODESTAKEHOLDER_H
