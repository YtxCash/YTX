#ifndef TREEWIDGETCOMMON_H
#define TREEWIDGETCOMMON_H

#include "component/info.h"
#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetCommon;
}

class TreeWidgetCommon final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override;

public:
    TreeWidgetCommon(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetCommon() override;

    QPointer<QTreeView> View() override;
    QPointer<TreeModel> Model() override { return model_; };
    void SetStatus() override;

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidgetCommon* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSettings& settings_ {};

    bool equal_unit { false };
};

#endif // TREEWIDGETCOMMON_H
