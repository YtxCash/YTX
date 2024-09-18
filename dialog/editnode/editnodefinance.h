#ifndef EDITNODEFIANNCE_H
#define EDITNODEFIANNCE_H

#include <QDialog>

#include "component/info.h"
#include "component/using.h"
#include "tree/model/abstracttreemodel.h"
#include "tree/node.h"

namespace Ui {
class EditNodeFinance;
}

class EditNodeFinance final : public QDialog {
    Q_OBJECT

public:
    EditNodeFinance(Node* node, CString& separator, CInfo& info, const AbstractTreeModel& model, int parent_id, bool node_usage, bool view_opened,
        QWidget* parent = nullptr);
    ~EditNodeFinance();

private slots:
    void REditName(const QString& arg1);

    void on_lineName_editingFinished();
    void on_lineCode_editingFinished();
    void on_lineDescription_editingFinished();
    void on_comboUnit_currentIndexChanged(int index);
    void on_rBtnDDCI_toggled(bool checked);
    void on_chkBoxBranch_toggled(bool checked);
    void on_plainNote_textChanged();

private:
    void IniDialog(CStringHash& unit_hash);
    void IniConnect();
    void Data(Node* node);

private:
    Ui::EditNodeFinance* ui;
    Node* node_ {};
    CString& separator_ {};

    const AbstractTreeModel& model_;
    int parent_id_ {};

    bool node_usage_ {};
    bool view_opened_ {};
    QStringList name_list_ {};
    QString parent_path_ {};
};

#endif // EDITNODEFIANNCE_H
