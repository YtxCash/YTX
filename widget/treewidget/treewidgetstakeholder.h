#ifndef TREEWIDGETSTAKEHOLDER_H
#define TREEWIDGETSTAKEHOLDER_H

#include "abstracttreewidget.h"
#include "component/info.h"
#include "component/settings.h"
#include "tree/model/abstracttreemodel.h"

namespace Ui {
class TreeWidgetStakeholder;
}

class TreeWidgetStakeholder final : public AbstractTreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override {};

public:
    TreeWidgetStakeholder(AbstractTreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidgetStakeholder() override;

    QTreeView* View() override;
    QHeaderView* Header() override;
    void SetCurrentIndex(const QModelIndex& index) override;

    void SetStatus() override {};

private:
    Ui::TreeWidgetStakeholder* ui;

    AbstractTreeModel* model_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};
};

#endif // TREEWIDGETSTAKEHOLDER_H
