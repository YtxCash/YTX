#include "tablewidgetcommon.h"

#include "ui_tablewidgetcommon.h"

TableWidgetCommon::TableWidgetCommon(TableModel* model, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetCommon)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

TableWidgetCommon::~TableWidgetCommon() { delete ui; }

QPointer<QTableView> TableWidgetCommon::View() const { return ui->tableView; }
