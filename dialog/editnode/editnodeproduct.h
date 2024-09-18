#ifndef EDITNODEPRODUCT_H
#define EDITNODEPRODUCT_H

#include <QDialog>

#include "component/settings.h"
#include "component/using.h"
#include "tree/model/abstracttreemodel.h"
#include "tree/node.h"

namespace Ui {
class EditNodeProduct;
}

class EditNodeProduct final : public QDialog {
    Q_OBJECT

public:
    EditNodeProduct(Node* node, CSectionRule& section_rule, CString& separator, CStringHash& unit_hash, const AbstractTreeModel& model, int parent_id,
        bool node_usage, bool view_opened, QWidget* parent = nullptr);
    ~EditNodeProduct();

private slots:
    void REditName(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_comboUnit_currentIndexChanged(int index);
    void on_rBtnDDCI_toggled(bool checked);
    void on_chkBoxBranch_toggled(bool checked);
    void on_plainTextEdit_textChanged();
    void on_dSpinBoxUnitPrice_editingFinished();
    void on_dSpinBoxCommission_editingFinished();

private:
    void IniDialog(CStringHash& currency_map);
    void IniConnect();
    void Data(Node* node);

private:
    Ui::EditNodeProduct* ui;
    Node* node_ {};
    CString& separator_ {};
    CSectionRule& section_rule_ {};

    const AbstractTreeModel& model_;
    int parent_id_ {};

    bool node_usage_ {};
    bool view_opened_ {};
    QStringList name_list_ {};
    QString parent_path_ {};
};

#endif // EDITNODEPRODUCT_H
