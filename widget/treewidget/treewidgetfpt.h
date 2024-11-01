#ifndef TREEWIDGETFPT_H
#define TREEWIDGETFPT_H

#include "component/info.h"
#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetFPT;
}

class TreeWidgetFPT final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override;

public:
    TreeWidgetFPT(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetFPT() override;

    QPointer<QTreeView> View() const override;
    QPointer<TreeModel> Model() const override { return model_; };
    void SetStatus() override;

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidgetFPT* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSettings& settings_ {};

    bool equal_unit { false };
};

#endif // TREEWIDGETFPT_H
