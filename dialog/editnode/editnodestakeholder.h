#ifndef EDITNODESTAKEHOLDER_H
#define EDITNODESTAKEHOLDER_H

#include <QDialog>

#include "component/info.h"
#include "component/settings.h"
#include "component/using.h"

namespace Ui {
class EditNodeStakeholder;
}

class EditNodeStakeholder final : public QDialog {
    Q_OBJECT

public:
    EditNodeStakeholder(Node* node, const SectionRule* section_rule, CString* separator, const Info* info, bool node_usage, bool view_opened,
        CString& parent_path, CStringHash* node_term_hash, CStringHash* branch_path, const NodeHash* node_hash, QWidget* parent = nullptr);
    ~EditNodeStakeholder();

private slots:
    void REditName(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_chkBoxBranch_toggled(bool checked);
    void on_plainTextEdit_textChanged();
    void on_comboMark_currentIndexChanged(int index);
    void on_comboTerm_currentIndexChanged(int index);
    void on_spinBoxPaymentPeriod_editingFinished();
    void on_dateEditDeadline_editingFinished();
    void on_dSpinBoxTaxRate_editingFinished();
    void on_comboEmployee_currentIndexChanged(int index);

protected:
    void changeEvent(QEvent* event) override;

private:
    void IniDialog(CStringHash* currency_map);
    void IniComboMark(bool branch, CStringHash* unit_hash);
    void IniComboEmployee();
    void IniConnect();
    void Data(Node* node);

private:
    Ui::EditNodeStakeholder* ui;
    Node* node_ {};
    CString* separator_ {};
    CStringHash* node_term_hash_ {};
    CStringHash* branch_path_ {};
    const SectionRule* section_rule_ {};
    const NodeHash* node_hash_ {};
    const Info* info_ {};

    bool node_usage_ {};
    bool view_opened_ {};
    QStringList node_name_list_ {};
    QString parent_path_ {};
};

#endif // EDITNODESTAKEHOLDER_H
