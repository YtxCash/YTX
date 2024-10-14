#ifndef EDITNODEPRODUCT_H
#define EDITNODEPRODUCT_H

#include <QDialog>

#include "component/using.h"
#include "tree/node.h"

namespace Ui {
class EditNodeProduct;
}

class EditNodeProduct final : public QDialog {
    Q_OBJECT

public:
    EditNodeProduct(Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool branch_enable, bool unit_enable, int amount_decimal,
        QWidget* parent = nullptr);
    ~EditNodeProduct();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_dSpinBoxUnitPrice_editingFinished();
    void on_dSpinBoxCommission_editingFinished();

    void on_comboUnit_currentIndexChanged(int index);

    void on_rBtnDDCI_toggled(bool checked);
    void on_chkBoxBranch_toggled(bool checked);

    void on_plainTextEdit_textChanged();

private:
    void IniDialog(CStringHash& unit_hash, int amount_decimal);
    void IniConnect();
    void Data(Node* node, bool branch_enable, bool unit_enable);

private:
    Ui::EditNodeProduct* ui;
    Node* node_ {};

    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // EDITNODEPRODUCT_H
