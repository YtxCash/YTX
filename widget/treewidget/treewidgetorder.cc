#include "treewidgetorder.h"

#include <QApplication>

#include "ui_treewidgetorder.h"

TreeWidgetOrder::TreeWidgetOrder(AbstractTreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent)
    : AbstractTreeWidget(parent)
    , ui(new Ui::TreeWidgetOrder)
    , model_ { model }
    , info_ { info }
    , section_rule_ { section_rule }
{
    ui->setupUi(this);
    ui->treeViewOrder->setModel(model);
    ui->dateEditEnd->setDate(QDate::currentDate());
    ui->dateEditStart->setDate(QDate::currentDate());
}

TreeWidgetOrder::~TreeWidgetOrder() { delete ui; }

void TreeWidgetOrder::SetCurrentIndex(const QModelIndex& index) { ui->treeViewOrder->setCurrentIndex(index); }

QTreeView* TreeWidgetOrder::View() { return ui->treeViewOrder; }

QHeaderView* TreeWidgetOrder::Header() { return ui->treeViewOrder->header(); }

void TreeWidgetOrder::on_dateEditStart_dateChanged(const QDate& date) { }

void TreeWidgetOrder::on_dateEditEnd_dateChanged(const QDate& date) { }
