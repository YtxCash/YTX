#include "editnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "ui_editnodeorder.h"

EditNodeOrder::EditNodeOrder(Node* node, AbstractTreeModel* order_model, AbstractTreeModel* stakeholder_model, const AbstractTreeModel& product_model,
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

    if (bool branch { node->branch }) {
        SetWidgetsEnabledBranch(false);
        ui->labelParty->setText(branch ? tr("Branch") : tr("Party"));
        ui->comboParty->lineEdit()->setValidator(&LineEdit::GetInputValidator());
        ui->comboParty->lineEdit()->setText(node->name);
        ui->chkBoxBranch->setChecked(true);
    }

    if (!node->branch) {
        ui->comboParty->blockSignals(true);
        ui->comboEmployee->blockSignals(true);

        IniDialog();
        IniConnect();
        IniData();

        ui->comboParty->blockSignals(false);
        ui->comboEmployee->blockSignals(false);
    }

    ui->chkBoxBranch->setEnabled(false);
    if (node_->locked)
        SetWidgetsEnabledPost(false);

    ui->pBtnLockOrder->setText(node_->locked ? tr("UnLock") : tr("Lock"));
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
}

void EditNodeOrder::IniData()
{
    auto party_index { ui->comboParty->findData(node_->party) };
    ui->comboParty->setCurrentIndex(party_index);

    auto employee_index { ui->comboEmployee->findData(node_->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    switch (node_->unit) {
    case UNIT_CASH:
        ui->rBtnCash->setChecked(true);
        break;
    case UNIT_MONTHLY:
        ui->rBtnMonthly->setChecked(true);
        break;
    case UNIT_PENDING:
        ui->rBtnPending->setChecked(true);
        break;
    default:
        break;
    }

    ui->chkBoxRefund->setChecked(node_->node_rule);
    ui->dSpinInitialTotal->setValue(node_->initial_total);
    ui->dSpinFinalTotal->setValue(node_->final_total);
    ui->dSpinDiscount->setValue(node_->discount);
    ui->chkBoxBranch->setChecked(node_->branch);
    ui->lineDescription->setText(node_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(node_->date_time, DATE_TIME_FST));
    ui->pBtnLockOrder->setChecked(node_->locked);
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
    if (is_modified_) {
        order_model_->UpdateNode(node_);
        EnableSave(false);
        ui->pBtnSaveOrder->setEnabled(is_modified_);
    }
}

void EditNodeOrder::reject()
{
    if (is_modified_) {
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

void EditNodeOrder::SetWidgetsEnabled(bool enabled)
{
    ui->labelEmployee->setEnabled(enabled);
    ui->comboEmployee->setEnabled(enabled);

    ui->labelDiscount->setEnabled(enabled);
    ui->dSpinDiscount->setEnabled(enabled);

    ui->pBtnInsertParty->setEnabled(enabled);

    ui->dateTimeEdit->setEnabled(enabled);
    ui->chkBoxRefund->setEnabled(enabled);
    ui->labelInitialTotal->setEnabled(enabled);
    ui->dSpinInitialTotal->setEnabled(enabled);
}

void EditNodeOrder::SetWidgetsEnabledBranch(bool enabled)
{
    ui->labelEmployee->setEnabled(enabled);
    ui->comboEmployee->setEnabled(enabled);

    ui->labelDiscount->setEnabled(enabled);
    ui->dSpinDiscount->setEnabled(enabled);

    ui->pBtnInsertParty->setEnabled(enabled);

    ui->dateTimeEdit->setEnabled(enabled);
    ui->chkBoxRefund->setEnabled(enabled);
    ui->labelInitialTotal->setEnabled(enabled);
    ui->dSpinInitialTotal->setEnabled(enabled);
    ui->pBtnPrint->setEnabled(enabled);
    ui->dSpinFinalTotal->setEnabled(enabled);
}

void EditNodeOrder::SetWidgetsEnabledPost(bool enabled)
{
    ui->labelParty->setEnabled(enabled);
    ui->comboParty->setEnabled(enabled);

    ui->pBtnInsertParty->setEnabled(enabled);

    ui->labelInitialTotal->setEnabled(enabled);
    ui->dSpinInitialTotal->setEnabled(enabled);

    ui->labelDiscount->setEnabled(enabled);
    ui->dSpinDiscount->setEnabled(enabled);

    ui->labelEmployee->setEnabled(enabled);
    ui->comboEmployee->setEnabled(enabled);

    ui->rBtnCash->setEnabled(enabled);
    ui->rBtnMonthly->setEnabled(enabled);
    ui->rBtnPending->setEnabled(enabled);
    ui->dateTimeEdit->setEnabled(enabled);

    ui->chkBoxRefund->setEnabled(enabled);
    ui->pBtnPrint->setEnabled(!enabled);
}

void EditNodeOrder::ZeroSettlement()
{
    ui->dSpinDiscount->setValue(0.0);
    ui->dSpinFinalTotal->setValue(0.0);
}

void EditNodeOrder::EnableSave(bool enable)
{
    is_modified_ = enable;
    ui->pBtnSaveOrder->setEnabled(enable);
}

void EditNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;
    EnableSave(true);
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (node_->branch)
        return;

    auto party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    EnableSave(true);
    node_->party = party_id;

    auto employee_index { ui->comboEmployee->findData(stakeholder_model_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_model_->NodeRule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakeholder_model_->NodeRule(party_id) == 1);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->node_rule = checked; }

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    EnableSave(true);
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;
    EnableSave(true);
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ZeroSettlement();
    EnableSave(true);
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ZeroSettlement();
    EnableSave(true);
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
    EnableSave(true);
}

void EditNodeOrder::on_pBtnLockOrder_toggled(bool checked)
{
    node_->locked = checked;
    SetWidgetsEnabledPost(!checked);
    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    if (checked) {
        accept();
    }
}

void EditNodeOrder::on_lineDescription_textChanged(const QString& arg1)
{
    node_->description = arg1;
    EnableSave(true);
}

void EditNodeOrder::on_dSpinDiscount_valueChanged(double arg1)
{
    node_->initial_total = node_->initial_total + node_->discount - arg1;
    ui->dSpinInitialTotal->setValue(node_->initial_total);
    node_->discount = arg1;
    EnableSave(true);
}
