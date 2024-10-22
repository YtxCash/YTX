#ifndef TREEWIDGETORDER_H
#define TREEWIDGETORDER_H

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodelorder.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetOrder;
}

class TreeWidgetOrder final : public TreeWidget {
    Q_OBJECT

public slots:
    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

public:
    TreeWidgetOrder(TreeModel* model, CInfo& info, const Settings& settings, QWidget* parent = nullptr);
    ~TreeWidgetOrder() override;

    QPointer<QTreeView> View() const override;
    QPointer<TreeModel> Model() const override { return model_; };

private slots:
    void on_pBtnRefresh_clicked();

private:
    Ui::TreeWidgetOrder* ui;
    QDate start_ {};
    QDate end_ {};

    TreeModelOrder* model_ {};
    CInfo& info_;
    const Settings& settings_;
};

#endif // TREEWIDGETORDER_H
