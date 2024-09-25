#ifndef TREEWIDGETCOMMON_H
#define TREEWIDGETCOMMON_H

#include "treewidget.h"
#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodel.h"

namespace Ui {
class TreeWidgetCommon;
}

class TreeWidgetCommon final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override;

public:
    TreeWidgetCommon(TreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidgetCommon() override;

    QTreeView* View() override;
    QHeaderView* Header() override;
    void SetCurrentIndex(const QModelIndex& index) override;
    void SetStatus() override;

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidgetCommon* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};

    bool equal_unit { false };
};

#endif // TREEWIDGETCOMMON_H
