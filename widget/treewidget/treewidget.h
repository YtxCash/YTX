#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include "abstracttreewidget.h"
#include "component/info.h"
#include "component/settings.h"
#include "tree/model/abstracttreemodel.h"

namespace Ui {
class TreeWidget;
}

class TreeWidget final : public AbstractTreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override;

public:
    TreeWidget(AbstractTreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidget() override;

    QTreeView* View() override;
    QHeaderView* Header() override;
    void SetCurrentIndex(const QModelIndex& index) override;
    void SetStatus() override;

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidget* ui;

    AbstractTreeModel* model_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};

    bool equal_unit { false };
};

#endif // TREEWIDGET_H
