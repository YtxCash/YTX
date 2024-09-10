#ifndef TREEWIDGETORDER_H
#define TREEWIDGETORDER_H

#include "abstracttreewidget.h"
#include "component/info.h"
#include "component/settings.h"
#include "tree/model/abstracttreemodel.h"

namespace Ui {
class TreeWidgetOrder;
}

class TreeWidgetOrder final : public AbstractTreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override {};

    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

public:
    TreeWidgetOrder(AbstractTreeModel* model, CInfo& info, const SectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidgetOrder() override;

    QTreeView* View() override;
    QHeaderView* Header() override;
    void SetCurrentIndex(const QModelIndex& index) override;

    void SetStatus() override {};

private:
    Ui::TreeWidgetOrder* ui;

    AbstractTreeModel* model_ {};
    CInfo& info_;
    const SectionRule& section_rule_;
};

#endif // TREEWIDGETORDER_H
