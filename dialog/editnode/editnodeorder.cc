#include "editnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "dialog/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_editnodeorder.h"

EditNodeOrder::EditNodeOrder(
    NodeShadow* node_shadow, SPSqlite sql, TableModel* order_table, TreeModel* stakeholder_tree, int value_decimal, int unit_party, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeOrder)
    , node_shadow_ { node_shadow }
    , sql_ { sql }
    , unit_party_ { unit_party }
    , value_decimal_ { value_decimal }
    , order_table_ { order_table }
    , stakeholder_tree_ { stakeholder_tree }
    , info_node_ { unit_party == UNIT_CUSTOMER ? SALES : PURCHASE }
    , node_id_ { *node_shadow->id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    bool branch { *node_shadow_->branch };

    if (branch) {
        ui->comboParty->lineEdit()->setValidator(&LineEdit::GetInputValidator());
        ui->comboParty->lineEdit()->setText(*node_shadow_->name);
        ui->chkBoxBranch->setChecked(true);
        UpdateUnit(*node_shadow_->unit);

        ui->pBtnLockOrder->setChecked(*node_shadow_->locked);
    }

    if (!branch) {
        IniDialog();
        IniData();
    }

    IniConnect();

    ui->chkBoxBranch->setEnabled(false);

    ui->pBtnLockOrder->setText(*node_shadow_->locked ? tr("UnLock") : tr("Lock"));
    ui->labelParty->setText(branch ? tr("Branch") : tr("Party"));

    LockWidgets(*node_shadow_->locked, branch);
}

EditNodeOrder::~EditNodeOrder() { delete ui; }

void EditNodeOrder::RUpdateStakeholder()
{
    if (*node_shadow_->branch)
        return;

    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_tree_->ComboPathLeafUnit(ui->comboEmployee, UNIT_EMPLOYEE);
    stakeholder_tree_->ComboPathLeafUnit(ui->comboParty, unit_party_);

    ui->comboEmployee->model()->sort(0);
    ui->comboParty->model()->sort(0);

    auto index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    auto index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

void EditNodeOrder::RUpdateLocked(int node_id, bool checked)
{
    if (node_id != node_id_)
        return;

    ui->pBtnLockOrder->blockSignals(true);
    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));
    ui->pBtnLockOrder->setChecked(checked);
    ui->pBtnLockOrder->blockSignals(false);

    LockWidgets(checked, *node_shadow_->branch);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

TableView* EditNodeOrder::View() { return ui->tableViewOrder; }

void EditNodeOrder::IniDialog()
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
    ui->dSpinFirst->setRange(IMIN, IMAX);
    ui->tableViewOrder->setFocus();
}

void EditNodeOrder::IniData()
{
    auto party_index { ui->comboParty->findData(*node_shadow_->party) };
    ui->comboParty->setCurrentIndex(party_index);

    auto employee_index { ui->comboEmployee->findData(*node_shadow_->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->dSpinInitialTotal->setValue(*node_shadow_->initial_total);
    ui->dSpinDiscount->setValue(*node_shadow_->discount);
    UpdateUnit(*node_shadow_->unit);

    ui->chkBoxRefund->setChecked(*node_shadow_->rule);
    ui->chkBoxBranch->setChecked(*node_shadow_->branch);
    ui->lineDescription->setText(*node_shadow_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(*node_shadow_->date_time, DATE_TIME_FST));
    ui->pBtnLockOrder->setChecked(*node_shadow_->locked);
    ui->dSpinFirst->setValue(*node_shadow_->first);
    ui->dSpinSecond->setValue(*node_shadow_->second);

    ui->tableViewOrder->setModel(order_table_);
    order_table_->insertRows(order_table_->rowCount(), 1);
    QModelIndex target_index { order_table_->index(order_table_->rowCount(), std::to_underlying(TableEnum::kDateTime)) };
    ui->tableViewOrder->setCurrentIndex(target_index);
}

void EditNodeOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_tree_->ComboPathLeafUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void EditNodeOrder::accept()
{
    if (auto focus_widget { this->focusWidget() })
        focus_widget->clearFocus();
}

void EditNodeOrder::IniConnect() { connect(ui->pBtnLockOrder, &QPushButton::clicked, this, &EditNodeOrder::accept); }

void EditNodeOrder::LockWidgets(bool locked, bool branch)
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
    ui->tableViewOrder->setEnabled(not_branch_enable);

    ui->rBtnCash->setEnabled(not_branch_enable);
    ui->rBtnMonthly->setEnabled(not_branch_enable);
    ui->rBtnPending->setEnabled(not_branch_enable);
    ui->dateTimeEdit->setEnabled(not_branch_enable);

    ui->chkBoxRefund->setEnabled(not_branch_enable);
    ui->lineDescription->setEnabled(basic_enable);

    ui->pBtnPrint->setEnabled(locked && !branch);
}

void EditNodeOrder::UpdateUnit(int unit)
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

void EditNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!*node_shadow_->branch || arg1.isEmpty())
        return;

    *node_shadow_->name = arg1;
    sql_->UpdateField(info_node_, arg1, NAME, node_id_);
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (*node_shadow_->branch)
        return;

    auto party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    *node_shadow_->party = party_id;
    sql_->UpdateField(info_node_, party_id, PARTY, node_id_);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    auto employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == 1);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked)
{
    *node_shadow_->rule = checked;
    sql_->UpdateField(info_node_, checked, RULE, node_id_);
}

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    *node_shadow_->employee = ui->comboEmployee->currentData().toInt();
    sql_->UpdateField(info_node_, *node_shadow_->employee, EMPLOYEE, node_id_);
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_CASH;

    ui->dSpinFinalTotal->setValue(ui->dSpinInitialTotal->value() - ui->dSpinDiscount->value());
    *node_shadow_->final_total = ui->dSpinFinalTotal->value();

    sql_->UpdateField(info_node_, UNIT_CASH, UNIT, node_id_);
    sql_->UpdateField(info_node_, *node_shadow_->final_total, FINAL_TOTAL, node_id_);
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_MONTHLY;
    sql_->UpdateField(info_node_, UNIT_MONTHLY, UNIT, node_id_);

    ui->dSpinFinalTotal->setValue(0.0);
    *node_shadow_->final_total = 0.0;
    sql_->UpdateField(info_node_, 0.0, FINAL_TOTAL, node_id_);
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_PENDING;
    sql_->UpdateField(info_node_, UNIT_PENDING, UNIT, node_id_);

    ui->dSpinFinalTotal->setValue(0.0);
    *node_shadow_->final_total = 0.0;
    sql_->UpdateField(info_node_, 0.0, FINAL_TOTAL, node_id_);
}

void EditNodeOrder::on_pBtnInsertParty_clicked()
{
    auto name { ui->comboParty->currentText() };
    if (*node_shadow_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = stakeholder_tree_->Rule(-1);
    stakeholder_tree_->SetParent(node, -1);
    node->name = name;

    node->unit = unit_party_;

    stakeholder_tree_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void EditNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    *node_shadow_->date_time = date_time.toString(DATE_TIME_FST);
    sql_->UpdateField(info_node_, *node_shadow_->date_time, DATE_TIME, node_id_);
}

void EditNodeOrder::on_pBtnLockOrder_toggled(bool checked)
{
    *node_shadow_->locked = checked;
    sql_->UpdateField(info_node_, checked, LOCKED, node_id_);
    emit SUpdateLocked(node_id_, checked);

    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    LockWidgets(checked, *node_shadow_->branch);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void EditNodeOrder::on_dSpinInitialTotal_editingFinished()
{
    auto value { ui->dSpinInitialTotal->value() };

    *node_shadow_->final_total = value - *node_shadow_->discount;
    ui->dSpinFinalTotal->setValue(*node_shadow_->final_total);
    *node_shadow_->initial_total = value;

    sql_->UpdateField(info_node_, value, INITIAL_TOTAL, node_id_);
    sql_->UpdateField(info_node_, *node_shadow_->final_total, FINAL_TOTAL, node_id_);
}

void EditNodeOrder::on_dSpinDiscount_editingFinished()
{
    auto value { ui->dSpinDiscount->value() };

    if (*node_shadow_->rule == UNIT_CASH) {
        *node_shadow_->final_total = *node_shadow_->initial_total - value;
        ui->dSpinFinalTotal->setValue(*node_shadow_->final_total);
        sql_->UpdateField(info_node_, *node_shadow_->final_total, FINAL_TOTAL, node_id_);
    }

    *node_shadow_->discount = value;
    sql_->UpdateField(info_node_, value, DISCOUNT, node_id_);
}

void EditNodeOrder::on_dSpinFirst_editingFinished()
{
    *node_shadow_->first = ui->dSpinFirst->value();
    sql_->UpdateField(info_node_, *node_shadow_->first, FIRST, node_id_);
}
void EditNodeOrder::on_dSpinSecond_editingFinished()
{
    *node_shadow_->second = ui->dSpinSecond->value();
    sql_->UpdateField(info_node_, *node_shadow_->second, SECOND, node_id_);
}
void EditNodeOrder::on_lineDescription_editingFinished()
{
    *node_shadow_->description = ui->lineDescription->text();
    sql_->UpdateField(info_node_, *node_shadow_->description, DESCRIPTION, node_id_);
}
