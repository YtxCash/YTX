#include "tablewidgetorder.h"

#include "ui_tablewidgetorder.h"

TableWidgetOrder::TableWidgetOrder(QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetOrder)
{
    ui->setupUi(this);
}

TableWidgetOrder::~TableWidgetOrder() { delete ui; }

void TableWidgetOrder::SetModel(TableModel* model)
{
    ui->tableViewOrder->setModel(model);
    order_table_model_ = model;
}

QTableView* TableWidgetOrder::View() { return ui->tableViewOrder; }
