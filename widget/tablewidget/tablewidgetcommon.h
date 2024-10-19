#ifndef TABLEWIDGETCOMMON_H
#define TABLEWIDGETCOMMON_H

#include <QTableView>

#include "table/model/tablemodel.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetCommon;
}

class TableWidgetCommon final : public TableWidget {
    Q_OBJECT

public:
    explicit TableWidgetCommon(TableModel* model, QWidget* parent = nullptr);
    ~TableWidgetCommon();

    QPointer<TableModel> Model() override { return model_; }
    QPointer<QTableView> View() override;

private:
    Ui::TableWidgetCommon* ui;
    TableModel* model_ {};
};

#endif // TABLEWIDGETCOMMON_H
