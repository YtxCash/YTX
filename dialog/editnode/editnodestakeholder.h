#ifndef EDITNODESTAKEHOLDER_H
#define EDITNODESTAKEHOLDER_H

#include <QDialog>

#include "component/info.h"
#include "component/settings.h"
#include "component/using.h"
#include "tree/model/treemodel.h"

namespace Ui {
class EditNodeStakeholder;
}

class EditNodeStakeholder final : public QDialog {
    Q_OBJECT

public:
    EditNodeStakeholder(Node* node, CSectionRule& section_rule, CString& separator, CInfo& info, bool node_usage, bool view_opened, int parent_id,
        CStringHash& term_hash, TreeModel* model, QWidget* parent = nullptr);
    ~EditNodeStakeholder();

private slots:
    void REditName(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_chkBoxBranch_toggled(bool checked);
    void on_plainTextEdit_textChanged();
    void on_comboMark_currentIndexChanged(int index);
    void on_spinBoxPaymentPeriod_editingFinished();
    void on_dateEditDeadline_editingFinished();
    void on_dSpinBoxTaxRate_editingFinished();
    void on_comboEmployee_currentIndexChanged(int index);
    void on_rBtnMonthly_toggled(bool checked);

protected:
    void changeEvent(QEvent* event) override;

private:
    void IniDialog(CStringHash& currency_map);
    void IniComboMark(bool branch, CStringHash& mark_hash);
    void IniComboEmployee();
    void IniConnect();
    void Data(Node* node);
    void EnableEmployee(bool employee);

private:
    Ui::EditNodeStakeholder* ui;
    Node* node_ {};
    CString& separator_;
    CStringHash& term_hash_;
    CSectionRule& section_rule_;
    CInfo& info_;
    TreeModel* model_ {};
    int parent_id_ {};

    bool node_usage_ {};
    bool view_opened_ {};
    QStringList name_list_ {};
    QString parent_path_ {};
};

#endif // EDITNODESTAKEHOLDER_H
