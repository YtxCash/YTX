#ifndef TABLEWIDGETFPTS_H
#define TABLEWIDGETFPTS_H

#include <QTableView>

#include "table/model/tablemodel.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetFPTS;
}

class TableWidgetFPTS final : public TableWidget {
    Q_OBJECT

public:
    explicit TableWidgetFPTS(TableModel* model, QWidget* parent = nullptr);
    ~TableWidgetFPTS();

    QPointer<TableModel> Model() const override { return model_; }
    QPointer<QTableView> View() const override;

private:
    Ui::TableWidgetFPTS* ui;
    TableModel* model_ {};
};

#endif // TABLEWIDGETFPTS_H
