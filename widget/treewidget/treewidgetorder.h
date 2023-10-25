#ifndef TREEWIDGETORDER_H
#define TREEWIDGETORDER_H

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodel.h"
#include "widget/abstracttreewidget.h"

namespace Ui {
class TreeWidgetOrder;
}

class TreeWidgetOrder final : public AbstractTreeWidget {
    Q_OBJECT

public:
    TreeWidgetOrder(TreeModel* model, CInfo& info, const SectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidgetOrder() override;

    void SetCurrentIndex(const QModelIndex& index) override;
    void SetStatus() override;
    void HideStatus() override;

    QTreeView* View() override;
    QHeaderView* Header() override;

public slots:
    void RUpdateDSpinBox() override;

    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);

    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidgetOrder* ui;

    TreeModel* model_ {};
    CInfo& info_;
    const SectionRule& section_rule_;

    bool equal_unit { false };
};

#endif // TREEWIDGETORDER_H
