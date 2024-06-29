#ifndef EDITNODEPRODUCT_H
#define EDITNODEPRODUCT_H

#include <QDialog>

#include "component/settings.h"
#include "component/using.h"

namespace Ui {
class EditNodeProduct;
}

class EditNodeProduct final : public QDialog {
    Q_OBJECT

public:
    EditNodeProduct(Node* node, const SectionRule* section_rule, CString* separator, CStringHash* unit_hash, CString& parent_path, bool node_usage,
        bool view_opened, QWidget* parent = nullptr);
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

protected:
    void changeEvent(QEvent* event) override;

private:
    void IniDialog(CStringHash* currency_map);
    void IniConnect();
    void Data(Node* node);

private:
    Ui::EditNodeProduct* ui;
    Node* node_ {};
    CString* separator_ {};
    const SectionRule* section_rule_ {};

    bool node_usage_ {};
    bool view_opened_ {};
    QStringList node_name_list_ {};
    QString parent_path_ {};
};

#endif // EDITNODEPRODUCT_H
