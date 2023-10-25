#include "treewidget.h"

#include <QApplication>

#include "component/constvalue.h"
#include "ui_treewidget.h"

TreeWidget::TreeWidget(TreeModel* model, CInfo& info, CSectionRule& section_rule, QWidget* parent)
    : AbstractTreeWidget(parent)
    , ui(new Ui::TreeWidget)
    , model_ { model }
    , info_ { info }
    , section_rule_ { section_rule }
{
    ui->setupUi(this);
    ui->view_->setModel(model);
    ui->dspin_box_dynamic_->setRange(DMIN, DMAX);
    ui->dspin_box_static_->setRange(DMIN, DMAX);
    SetStatus();
}

TreeWidget::~TreeWidget() { delete ui; }

void TreeWidget::SetCurrentIndex(const QModelIndex& index) { ui->view_->setCurrentIndex(index); }

void TreeWidget::SetStatus()
{
    ui->dspin_box_static_->setDecimals(section_rule_.value_decimal);
    ui->lable_static_->setText(section_rule_.static_label);

    auto static_node_id { section_rule_.static_node };

    if (model_->Contains(static_node_id)) {
        ui->dspin_box_static_->setPrefix(info_.unit_symbol_hash.value(model_->Unit(static_node_id), QString()));
        StaticStatus(static_node_id);
    }

    ui->dspin_box_dynamic_->setDecimals(section_rule_.value_decimal);
    ui->label_dynamic_->setText(section_rule_.dynamic_label);

    auto dynamic_node_id_lhs { section_rule_.dynamic_node_lhs };
    auto dynamic_node_id_rhs { section_rule_.dynamic_node_rhs };

    if (model_->Contains(dynamic_node_id_lhs) && model_->Contains(dynamic_node_id_rhs)) {
        auto lhs_unit { model_->Unit(dynamic_node_id_lhs) };
        auto rhs_unit { model_->Unit(dynamic_node_id_rhs) };
        equal_unit = lhs_unit == rhs_unit;

        ui->dspin_box_dynamic_->setPrefix(info_.unit_symbol_hash.value((equal_unit ? lhs_unit : section_rule_.base_unit), QString()));
        DynamicStatus(dynamic_node_id_lhs, dynamic_node_id_rhs);
    }
}

void TreeWidget::HideStatus()
{
    ui->dspin_box_dynamic_->setHidden(true);
    ui->dspin_box_static_->setHidden(true);
}

QTreeView* TreeWidget::View() { return ui->view_; }

QHeaderView* TreeWidget::Header() { return ui->view_->header(); }

void TreeWidget::RUpdateDSpinBox()
{
    StaticStatus(section_rule_.static_node);
    DynamicStatus(section_rule_.dynamic_node_lhs, section_rule_.dynamic_node_rhs);
}

void TreeWidget::DynamicStatus(int lhs_node_id, int rhs_node_id)
{
    auto lhs_total { equal_unit ? model_->InitialTotal(lhs_node_id) : model_->FinalTotal(lhs_node_id) };
    auto rhs_total { equal_unit ? model_->InitialTotal(rhs_node_id) : model_->FinalTotal(rhs_node_id) };

    auto operation { section_rule_.operation.isEmpty() ? PLUS : section_rule_.operation };
    double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidget::StaticStatus(int node_id) { ui->dspin_box_static_->setValue(model_->InitialTotal(node_id)); }

double TreeWidget::Operate(double lhs, double rhs, const QString& operation)
{
    switch (operation.at(0).toLatin1()) {
    case '+':
        return lhs + rhs;
    case '-':
        return lhs - rhs;
    default:
        return 0.0;
    }
}
