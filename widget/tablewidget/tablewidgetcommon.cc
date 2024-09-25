#include "tablewidgetcommon.h"

#include "ui_tablewidgetcommon.h"

TableWidgetCommon::TableWidgetCommon(QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetCommon)
{
    ui->setupUi(this);
}

TableWidgetCommon::~TableWidgetCommon() { delete ui; }

void TableWidgetCommon::SetModel(TableModel* model)
{
    ui->tableView->setModel(model);
    model_ = model;
}

QTableView* TableWidgetCommon::View() { return ui->tableView; }
