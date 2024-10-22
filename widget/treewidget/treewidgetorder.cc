#include "treewidgetorder.h"

#include "ui_treewidgetorder.h"

TreeWidgetOrder::TreeWidgetOrder(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetOrder)
    , start_ { QDate::currentDate() }
    , end_ { QDate::currentDate() }
    , model_ { static_cast<TreeModelOrder*>(model) }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);

    ui->dateEditStart->setDisplayFormat(DATE_FST);
    ui->dateEditEnd->setDisplayFormat(DATE_FST);

    ui->dateEditStart->setDate(start_);
    ui->dateEditEnd->setDate(end_);

    model_->ConstructTreeOrder(start_, end_);
    ui->treeViewOrder->setModel(model);
}

TreeWidgetOrder::~TreeWidgetOrder() { delete ui; }

QPointer<QTreeView> TreeWidgetOrder::View() const { return ui->treeViewOrder; }

void TreeWidgetOrder::on_dateEditStart_dateChanged(const QDate& date)
{
    if (date > end_) {
        ui->pBtnRefresh->setEnabled(false);
        return;
    }

    ui->pBtnRefresh->setEnabled(true);
    start_ = date;
}

void TreeWidgetOrder::on_dateEditEnd_dateChanged(const QDate& date)
{
    if (date < start_) {
        ui->pBtnRefresh->setEnabled(false);
        return;
    }

    ui->pBtnRefresh->setEnabled(true);
    end_ = date;
}

void TreeWidgetOrder::on_pBtnRefresh_clicked() { model_->ConstructTreeOrder(start_, end_); }
