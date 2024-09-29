#include "treewidgetcommon.h"

#include <QApplication>

#include "component/constvalue.h"
#include "ui_treewidgetcommon.h"

TreeWidgetCommon::TreeWidgetCommon(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetCommon)
    , model_ { model }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);
    ui->treeView->setModel(model);
    ui->dspin_box_dynamic_->setRange(DMIN, DMAX);
    ui->dspin_box_static_->setRange(DMIN, DMAX);
    SetStatus();
}

TreeWidgetCommon::~TreeWidgetCommon() { delete ui; }

void TreeWidgetCommon::SetCurrentIndex(const QModelIndex& index) { ui->treeView->setCurrentIndex(index); }

void TreeWidgetCommon::SetStatus()
{
    ui->dspin_box_static_->setDecimals(settings_.value_decimal);
    ui->lable_static_->setText(settings_.static_label);

    auto static_node_id { settings_.static_node };

    if (model_->Contains(static_node_id)) {
        ui->dspin_box_static_->setPrefix(info_.unit_symbol_hash.value(model_->Unit(static_node_id), QString()));
        StaticStatus(static_node_id);
    }

    ui->dspin_box_dynamic_->setDecimals(settings_.value_decimal);
    ui->label_dynamic_->setText(settings_.dynamic_label);

    auto dynamic_node_id_lhs { settings_.dynamic_node_lhs };
    auto dynamic_node_id_rhs { settings_.dynamic_node_rhs };

    if (model_->Contains(dynamic_node_id_lhs) && model_->Contains(dynamic_node_id_rhs)) {
        auto lhs_unit { model_->Unit(dynamic_node_id_lhs) };
        auto rhs_unit { model_->Unit(dynamic_node_id_rhs) };
        equal_unit = lhs_unit == rhs_unit;

        ui->dspin_box_dynamic_->setPrefix(info_.unit_symbol_hash.value((equal_unit ? lhs_unit : settings_.base_unit), QString()));
        DynamicStatus(dynamic_node_id_lhs, dynamic_node_id_rhs);
    }
}

QTreeView* TreeWidgetCommon::View() { return ui->treeView; }

QHeaderView* TreeWidgetCommon::Header() { return ui->treeView->header(); }

void TreeWidgetCommon::RUpdateDSpinBox()
{
    StaticStatus(settings_.static_node);
    DynamicStatus(settings_.dynamic_node_lhs, settings_.dynamic_node_rhs);
}

void TreeWidgetCommon::DynamicStatus(int lhs_node_id, int rhs_node_id)
{
    auto lhs_total { equal_unit ? model_->InitialTotal(lhs_node_id) : model_->FinalTotal(lhs_node_id) };
    auto rhs_total { equal_unit ? model_->InitialTotal(rhs_node_id) : model_->FinalTotal(rhs_node_id) };

    auto operation { settings_.operation.isEmpty() ? PLUS : settings_.operation };
    double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidgetCommon::StaticStatus(int node_id) { ui->dspin_box_static_->setValue(model_->InitialTotal(node_id)); }

double TreeWidgetCommon::Operate(double lhs, double rhs, const QString& operation)
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
