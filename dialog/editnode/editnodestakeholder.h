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
    EditNodeStakeholder(Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool enable_branch, int ratio_decimal,
        TreeModel* model, QWidget* parent = nullptr);
    ~EditNodeStakeholder();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_spinBoxDeadline_editingFinished();
    void on_spinBoxPaymentPeriod_editingFinished();
    void on_dSpinBoxTaxRate_editingFinished();

    void on_chkBoxBranch_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);

    void on_comboUnit_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_plainTextEdit_textChanged();

private:
    void IniDialog(CStringHash& unit_hash, TreeModel* model, int ratio_decimal);
    void IniComboWithStringHash(QComboBox* combo, CStringHash& hash);
    void IniComboEmployee(TreeModel* model);
    void IniConnect();
    void Data(Node* node, bool enable_branch);

private:
    Ui::EditNodeStakeholder* ui;
    Node* node_ {};

    QString parent_path_ {};
    CStringList& name_list_ {};
};

#endif // EDITNODESTAKEHOLDER_H
