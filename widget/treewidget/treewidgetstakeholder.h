#ifndef TREEWIDGETSTAKEHOLDER_H
#define TREEWIDGETSTAKEHOLDER_H

#include "treewidget.h"
#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodel.h"

namespace Ui {
class TreeWidgetStakeholder;
}

class TreeWidgetStakeholder final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override {};

public:
    TreeWidgetStakeholder(TreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent = nullptr);
    ~TreeWidgetStakeholder() override;

    QTreeView* View() override;
    QHeaderView* Header() override;
    void SetCurrentIndex(const QModelIndex& index) override;

    void SetStatus() override {};

private:
    Ui::TreeWidgetStakeholder* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};
};

#endif // TREEWIDGETSTAKEHOLDER_H
