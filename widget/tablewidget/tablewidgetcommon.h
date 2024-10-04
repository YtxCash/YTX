#ifndef TABLEWIDGETCOMMON_H
#define TABLEWIDGETCOMMON_H

#include "table/model/tablemodel.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetCommon;
}

class TableWidgetCommon final : public TableWidget {
    Q_OBJECT

public:
    explicit TableWidgetCommon(QWidget* parent = nullptr);
    ~TableWidgetCommon();

    void SetModel(TableModel* model) override;

    TableModel* Model() override { return model_; }
    TableView* View() override;

private:
    Ui::TableWidgetCommon* ui;
    TableModel* model_ {};
};

#endif // TABLEWIDGETCOMMON_H
