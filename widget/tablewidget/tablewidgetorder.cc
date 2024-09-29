#include "tablewidgetorder.h"

#include <QMessageBox>

#include "global/resourcepool.h"
#include "ui_tablewidgetorder.h"

TableWidgetOrder::TableWidgetOrder(
    Node* node, TreeModel* order_model, TreeModel* stakeholder_model, const TreeModel* product_model, int value_decimal, int unit_party, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetOrder)
    , node_ { node }
    , unit_party_ { unit_party }
    , value_decimal_ { value_decimal }
    , stakeholder_model_ { stakeholder_model }
    , order_model_ { order_model }
    , product_model_ { product_model }
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

void TableWidgetOrder::RAccept()
{
    if (auto focus_widget { this->focusWidget() })
        focus_widget->clearFocus();

    order_model_->UpdateNode(node_);
    ui->pBtnSaveOrder->setEnabled(false);
}

void TableWidgetOrder::RReject()
{
    if (ui->pBtnSaveOrder->isEnabled()) {
        auto reply { QMessageBox::question(
            this, tr("Save Modified"), tr("Have unsaved modifications. Save them ?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) };

        switch (reply) {
        case QMessageBox::Cancel:
            return;
        case QMessageBox::Yes:
            RAccept();
            break;
        case QMessageBox::No:
        default:
            break;
        }
    }
}

void TableWidgetOrder::RUpdateStakeholder()
{
    if (node_->branch)
        return;

    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_model_->ComboPathLeafUnit(ui->comboEmployee, UNIT_EMPLOYEE);
    stakeholder_model_->ComboPathLeafUnit(ui->comboParty, unit_party_);

    ui->comboEmployee->model()->sort(0);
    ui->comboParty->model()->sort(0);

    auto index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    auto index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

void TableWidgetOrder::RUpdateOrder(const QVariant& value, TreeEnumOrder column)
{
    switch (column) {
    case TreeEnumOrder::kDescription:
        ui->lineDescription->setText(value.toString());
        break;
    case TreeEnumOrder::kRule:
        ui->chkBoxRefund->setChecked(value.toBool());
        break;
    case TreeEnumOrder::kUnit: {
        UpdateUnit(value.toInt());
    } break;
    case TreeEnumOrder::kParty: {
        auto index_party { ui->comboParty->findData(value.toInt()) };
        ui->comboParty->setCurrentIndex(index_party);
    } break;
    case TreeEnumOrder::kEmployee: {
        auto employee_index { ui->comboEmployee->findData(value.toInt()) };
        ui->comboEmployee->setCurrentIndex(employee_index);
    } break;
    case TreeEnumOrder::kDateTime:
        ui->dateTimeEdit->setDateTime(QDateTime::fromString(value.toString(), DATE_TIME_FST));
        break;
    case TreeEnumOrder::kLocked:
        ui->pBtnLockOrder->setChecked(value.toBool());
        break;
    default:
        break;
    }

    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::IniDialog()
{
    IniCombo(ui->comboParty, unit_party_);
    IniCombo(ui->comboEmployee, UNIT_EMPLOYEE);

    ui->dateTimeEdit->setDisplayFormat(DATE_TIME_FST);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinDiscount->setDecimals(value_decimal_);
    ui->dSpinFinalTotal->setDecimals(value_decimal_);
    ui->dSpinFinalTotal->setRange(DMIN, DMAX);
    ui->dSpinInitialTotal->setDecimals(value_decimal_);
    ui->dSpinInitialTotal->setRange(DMIN, DMAX);
    ui->dSpinSecond->setRange(DMIN, DMAX);
    ui->dSpinSecond->setDecimals(value_decimal_);
    ui->spinFirst->setRange(IMIN, IMAX);
}

void TableWidgetOrder::IniData()
{
    auto party_index { ui->comboParty->findData(node_->party) };
    ui->comboParty->setCurrentIndex(party_index);

    auto employee_index { ui->comboEmployee->findData(node_->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->dSpinInitialTotal->setValue(node_->initial_total);
    ui->dSpinDiscount->setValue(node_->discount);
    UpdateUnit(node_->unit);

    ui->chkBoxRefund->setChecked(node_->rule);
    ui->chkBoxBranch->setChecked(node_->branch);
    ui->lineDescription->setText(node_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(node_->date_time, DATE_TIME_FST));
    ui->pBtnLockOrder->setChecked(node_->locked);
    ui->spinFirst->setValue(node_->first);
    ui->dSpinSecond->setValue(node_->second);
}

void TableWidgetOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_model_->ComboPathLeafUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void TableWidgetOrder::LockWidgets(bool locked, bool branch)
{
    bool basic_enable { !locked };
    bool not_branch_enable { !locked && !branch };

    ui->labelParty->setEnabled(basic_enable);
    ui->comboParty->setEnabled(basic_enable);

    ui->pBtnInsertParty->setEnabled(not_branch_enable);

    ui->labelInitialTotal->setEnabled(not_branch_enable);
    ui->dSpinInitialTotal->setEnabled(not_branch_enable);

    ui->dSpinFinalTotal->setEnabled(!branch);

    ui->labelDiscount->setEnabled(not_branch_enable);
    ui->dSpinDiscount->setEnabled(not_branch_enable);

    ui->labelEmployee->setEnabled(not_branch_enable);
    ui->comboEmployee->setEnabled(not_branch_enable);

    ui->rBtnCash->setEnabled(not_branch_enable);
    ui->rBtnMonthly->setEnabled(not_branch_enable);
    ui->rBtnPending->setEnabled(not_branch_enable);
    ui->dateTimeEdit->setEnabled(not_branch_enable);

    ui->chkBoxRefund->setEnabled(not_branch_enable);
    ui->lineDescription->setEnabled(basic_enable);

    ui->pBtnPrint->setEnabled(locked && !branch);
}

void TableWidgetOrder::UpdateUnit(int unit)
{
    switch (unit) {
    case UNIT_CASH:
        ui->rBtnCash->setChecked(true);
        ui->dSpinFinalTotal->setValue(ui->dSpinInitialTotal->value() - ui->dSpinDiscount->value());
        break;
    case UNIT_MONTHLY:
        ui->dSpinFinalTotal->setValue(0.0);
        ui->rBtnMonthly->setChecked(true);
        break;
    case UNIT_PENDING:
        ui->dSpinFinalTotal->setValue(0.0);
        ui->rBtnPending->setChecked(true);
        break;
    default:
        break;
    }
}

void TableWidgetOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;
    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (node_->branch)
        return;

    auto party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    ui->pBtnSaveOrder->setEnabled(true);
    node_->party = party_id;

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    auto employee_index { ui->comboEmployee->findData(stakeholder_model_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_model_->Rule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakeholder_model_->Rule(party_id) == 1);
}

void TableWidgetOrder::on_chkBoxRefund_toggled(bool checked) { node_->rule = checked; }

void TableWidgetOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;
    ui->dSpinFinalTotal->setValue(ui->dSpinInitialTotal->value() - ui->dSpinDiscount->value());
    node_->final_total = ui->dSpinFinalTotal->value();
    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ui->dSpinFinalTotal->setValue(0.0);
    node_->final_total = 0.0;
    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ui->dSpinFinalTotal->setValue(0.0);
    node_->final_total = 0.0;
    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::on_pBtnInsertParty_clicked()
{
    auto name { ui->comboParty->currentText() };
    if (node_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = stakeholder_model_->Rule(-1);
    stakeholder_model_->SetParent(node, -1);
    node->name = name;

    node->unit = unit_party_;

    stakeholder_model_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void TableWidgetOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(DATE_TIME_FST);
    ui->pBtnSaveOrder->setEnabled(true);
}

void TableWidgetOrder::on_pBtnLockOrder_toggled(bool checked)
{
    node_->locked = checked;
    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    LockWidgets(checked, node_->branch);
    order_model_->UpdateNodeLocked(node_);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void TableWidgetOrder::on_dSpinInitialTotal_editingFinished()
{
    auto value { ui->dSpinInitialTotal->value() };

    node_->final_total = value - node_->discount;
    ui->dSpinFinalTotal->setValue(node_->final_total);
    node_->initial_total = value;
}

void TableWidgetOrder::on_dSpinDiscount_editingFinished()
{
    auto value { ui->dSpinDiscount->value() };

    if (node_->rule == UNIT_CASH) {
        node_->final_total = node_->initial_total - value;
        ui->dSpinFinalTotal->setValue(node_->final_total);
    }

    node_->discount = value;
}

void TableWidgetOrder::on_spinFirst_editingFinished() { node_->first = ui->spinFirst->value(); }
void TableWidgetOrder::on_dSpinSecond_editingFinished() { node_->second = ui->dSpinSecond->value(); }
void TableWidgetOrder::on_lineDescription_editingFinished() { node_->description = ui->lineDescription->text(); }

void TableWidgetOrder::on_dSpinInitialTotal_valueChanged(double /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void TableWidgetOrder::on_dSpinDiscount_valueChanged(double /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void TableWidgetOrder::on_spinFirst_valueChanged(int /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void TableWidgetOrder::on_dSpinSecond_valueChanged(double /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void TableWidgetOrder::on_lineDescription_textChanged(const QString& /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
