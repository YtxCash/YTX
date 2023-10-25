#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodel.h"
#include "widget/abstracttreewidget.h"

namespace Ui {
class TreeWidget;
}

class TreeWidget final : public AbstractTreeWidget {
    Q_OBJECT

public:
    TreeWidget(TreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidget() override;

    void SetCurrentIndex(const QModelIndex& index) override;
    void SetStatus() override;
    void HideStatus() override;

    QTreeView* View() override;
    QHeaderView* Header() override;

public slots:
    void RUpdateDSpinBox() override;

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);

    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidget* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};

    bool equal_unit { false };
};

#endif // TREEWIDGET_H
