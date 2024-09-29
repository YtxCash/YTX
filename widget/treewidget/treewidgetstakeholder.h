#ifndef TREEWIDGETSTAKEHOLDER_H
#define TREEWIDGETSTAKEHOLDER_H

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodel.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetStakeholder;
}

class TreeWidgetStakeholder final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override { };

public:
    TreeWidgetStakeholder(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetStakeholder() override;

    QTreeView* View() override;
    QHeaderView* Header() override;
    void SetCurrentIndex(const QModelIndex& index) override;

    void SetStatus() override { };

private:
    Ui::TreeWidgetStakeholder* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSettings& settings_ {};
};

#endif // TREEWIDGETSTAKEHOLDER_H
