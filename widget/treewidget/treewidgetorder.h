#ifndef TREEWIDGETORDER_H
#define TREEWIDGETORDER_H

#include "component/info.h"
#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetOrder;
}

class TreeWidgetOrder final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override { };

    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

public:
    TreeWidgetOrder(TreeModel* model, CInfo& info, const Settings& settings, QWidget* parent = nullptr);
    ~TreeWidgetOrder() override;

    QTreeView* View() override;
    TreeModel* Model() override { return model_; };
    void SetStatus() override { };

private:
    Ui::TreeWidgetOrder* ui;

    TreeModel* model_ {};
    CInfo& info_;
    const Settings& settings_;
};

#endif // TREEWIDGETORDER_H
