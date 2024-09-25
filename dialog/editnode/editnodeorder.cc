#include "editnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "dialog/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_editnodeorder.h"

EditNodeOrder::EditNodeOrder(Node* node, TreeModel* order_model, TreeModel* stakeholder_model, const TreeModel& product_model,
    int value_decimal, int unit_party, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeOrder)
    , node_ { node }
    , unit_party_ { unit_party }
    , value_decimal_ { value_decimal }
    , stakeholder_model_ { stakeholder_model }
    , order_model_ { order_model }
    , product_model_ { product_model }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    bool branch { node->branch };

    if (branch) {
        ui->comboParty->lineEdit()->setValidator(&LineEdit::GetInputValidator());
        ui->comboParty->lineEdit()->setText(node->name);
        ui->chkBoxBranch->setChecked(true);
        UpdateUnit(node_->unit);

        ui->pBtnLockOrder->setChecked(node->locked);
    }

    if (!branch) {
        IniDialog();
        IniData();
    }

    IniConnect();

    ui->chkBoxBranch->setEnabled(false);
    ui->pBtnSaveOrder->setEnabled(false);

    ui->pBtnLockOrder->setText(node_->locked ? tr("UnLock") : tr("Lock"));
    ui->labelParty->setText(branch ? tr("Branch") : tr("Party"));

    LockWidgets(node->locked, branch);
}

EditNodeOrder::~EditNodeOrder() { delete ui; }

void EditNodeOrder::RUpdateStakeholder()
{
    if (node_->branch)
        return;

    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_model_->ComboPathUnit(ui->comboEmployee, UNIT_EMPLOYEE);
    stakeholder_model_->ComboPathUnit(ui->comboParty, unit_party_);

    ui->comboEmployee->model()->sort(0);
    ui->comboParty->model()->sort(0);

    auto index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    auto index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

void EditNodeOrder::RUpdateOrder(const QVariant& value, TreeEnumOrder column)
{
    switch (column) {
    case TreeEnumOrder::kDescription:
        ui->lineDescription->setText(value.toString());
        break;
    case TreeEnumOrder::kNodeRule:
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
    ui->spinFirst->setRange(IMIN, IMAX);
}

void EditNodeOrder::IniData()
{
    auto party_index { ui->comboParty->findData(node_->party) };
    ui->comboParty->setCurrentIndex(party_index);

    auto employee_index { ui->comboEmployee->findData(node_->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->dSpinInitialTotal->setValue(node_->initial_total);
    ui->dSpinDiscount->setValue(node_->discount);
    UpdateUnit(node_->unit);

    ui->chkBoxRefund->setChecked(node_->node_rule);
    ui->chkBoxBranch->setChecked(node_->branch);
    ui->lineDescription->setText(node_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(node_->date_time, DATE_TIME_FST));
    ui->pBtnLockOrder->setChecked(node_->locked);
    ui->spinFirst->setValue(node_->first);
    ui->dSpinSecond->setValue(node_->second);
}

void EditNodeOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_model_->ComboPathUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void EditNodeOrder::accept()
{
    if (auto focus_widget { this->focusWidget() })
        focus_widget->clearFocus();

    order_model_->UpdateNode(node_);
    ui->pBtnSaveOrder->setEnabled(false);
}

void EditNodeOrder::reject()
{
    if (ui->pBtnSaveOrder->isEnabled()) {
        auto reply { QMessageBox::question(
            this, tr("Save Modified"), tr("Have unsaved modifications. Save them ?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) };

        switch (reply) {
        case QMessageBox::Cancel:
            return;
        case QMessageBox::Yes:
            accept();
            break;
        case QMessageBox::No:
        default:
            break;
        }
    }

    QDialog::reject();
}

void EditNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &EditNodeOrder::accept); }

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
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;
    ui->pBtnSaveOrder->setEnabled(true);
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
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

    ui->rBtnCash->setChecked(stakeholder_model_->NodeRule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakeholder_model_->NodeRule(party_id) == 1);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->node_rule = checked; }

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    ui->pBtnSaveOrder->setEnabled(true);
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;
    ui->dSpinFinalTotal->setValue(ui->dSpinInitialTotal->value() - ui->dSpinDiscount->value());
    node_->final_total = ui->dSpinFinalTotal->value();
    ui->pBtnSaveOrder->setEnabled(true);
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ui->dSpinFinalTotal->setValue(0.0);
    node_->final_total = 0.0;
    ui->pBtnSaveOrder->setEnabled(true);
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ui->dSpinFinalTotal->setValue(0.0);
    node_->final_total = 0.0;
    ui->pBtnSaveOrder->setEnabled(true);
}

void EditNodeOrder::on_pBtnInsertParty_clicked()
{
    auto name { ui->comboParty->currentText() };
    if (node_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->node_rule = stakeholder_model_->NodeRule(-1);
    stakeholder_model_->SetParent(node, -1);
    node->name = name;

    node->unit = unit_party_;

    stakeholder_model_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void EditNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(DATE_TIME_FST);
    ui->pBtnSaveOrder->setEnabled(true);
}

void EditNodeOrder::on_pBtnLockOrder_toggled(bool checked)
{
    node_->locked = checked;
    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    LockWidgets(checked, node_->branch);
    order_model_->UpdateNodeLocked(node_);

    if (checked) {
        if (ui->pBtnSaveOrder->isEnabled())
            accept();

        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void EditNodeOrder::on_dSpinInitialTotal_editingFinished()
{
    auto value { ui->dSpinInitialTotal->value() };

    node_->final_total = value - node_->discount;
    ui->dSpinFinalTotal->setValue(node_->final_total);
    node_->initial_total = value;
}

void EditNodeOrder::on_dSpinDiscount_editingFinished()
{
    auto value { ui->dSpinDiscount->value() };

    if (node_->node_rule == UNIT_CASH) {
        node_->final_total = node_->initial_total - value;
        ui->dSpinFinalTotal->setValue(node_->final_total);
    }

    node_->discount = value;
}

void EditNodeOrder::on_spinFirst_editingFinished() { node_->first = ui->spinFirst->value(); }
void EditNodeOrder::on_dSpinSecond_editingFinished() { node_->second = ui->dSpinSecond->value(); }
void EditNodeOrder::on_lineDescription_editingFinished() { node_->description = ui->lineDescription->text(); }

void EditNodeOrder::on_dSpinInitialTotal_valueChanged(double /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void EditNodeOrder::on_dSpinDiscount_valueChanged(double /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void EditNodeOrder::on_spinFirst_valueChanged(int /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void EditNodeOrder::on_dSpinSecond_valueChanged(double /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
void EditNodeOrder::on_lineDescription_textChanged(const QString& /*arg1*/) { ui->pBtnSaveOrder->setEnabled(true); }
