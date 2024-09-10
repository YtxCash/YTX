#include "treewidgetstakeholder.h"

#include <QApplication>

#include "ui_treewidgetstakeholder.h"

TreeWidgetStakeholder::TreeWidgetStakeholder(AbstractTreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent)
    : AbstractTreeWidget(parent)
    , ui(new Ui::TreeWidgetStakeholder)
    , model_ { model }
    , info_ { info }
    , section_rule_ { section_rule }
{
    ui->setupUi(this);
    ui->treeViewStakeholder->setModel(model);
}

TreeWidgetStakeholder::~TreeWidgetStakeholder() { delete ui; }

void TreeWidgetStakeholder::SetCurrentIndex(const QModelIndex& index) { ui->treeViewStakeholder->setCurrentIndex(index); }

QTreeView* TreeWidgetStakeholder::View() { return ui->treeViewStakeholder; }

QHeaderView* TreeWidgetStakeholder::Header() { return ui->treeViewStakeholder->header(); }
