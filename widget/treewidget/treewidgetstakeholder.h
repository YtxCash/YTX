#ifndef TREEWIDGETSTAKEHOLDER_H
#define TREEWIDGETSTAKEHOLDER_H

#include "component/info.h"
#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetStakeholder;
}

class TreeWidgetStakeholder final : public TreeWidget {
    Q_OBJECT

public:
    TreeWidgetStakeholder(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetStakeholder() override;

    QTreeView* View() override;
    TreeModel* Model() override { return model_; };

private:
    Ui::TreeWidgetStakeholder* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSettings& settings_ {};
};

#endif // TREEWIDGETSTAKEHOLDER_H
