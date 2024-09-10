#include "tablewidget.h"

#include "ui_tablewidget.h"

TableWidget::TableWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TableWidget)
{
    ui->setupUi(this);
}

TableWidget::~TableWidget() { delete ui; }

void TableWidget::SetModel(TableModel* model)
{
    ui->tableView->setModel(model);
    model_ = model;
}

QTableView* TableWidget::View() { return ui->tableView; }
