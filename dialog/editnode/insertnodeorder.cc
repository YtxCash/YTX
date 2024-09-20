#include "insertnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "ui_insertnodeorder.h"

InsertNodeOrder::InsertNodeOrder(Node* node, CSectionRule& section_rule, AbstractTreeModel* order_model, AbstractTreeModel* stakeholder_model,
    const AbstractTreeModel& product_model, int unit_party, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeOrder)
    , node_ { node }
    , unit_party_ { unit_party }
    , section_rule_ { section_rule }
    , stakeholder_model_ { stakeholder_model }
    , order_model_ { order_model }
    , product_model_ { product_model }
{
    ui->setupUi(this);

    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    IniDialog();
    IniConnect();

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

InsertNodeOrder::~InsertNodeOrder() { delete ui; }

void InsertNodeOrder::RUpdateStakeholder()
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

void InsertNodeOrder::IniDialog()
{
    IniCombo(ui->comboParty, unit_party_);
    IniCombo(ui->comboEmployee, UNIT_EMPLOYEE);

    ui->dateTimeEdit->setDisplayFormat(DATE_TIME_FST);
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());

    ui->pBtnLockOrder->setChecked(false);
    ui->pBtnLockOrder->setText(tr("Lock"));

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinDiscount->setDecimals(section_rule_.value_decimal);
    ui->dSpinFinalTotal->setDecimals(section_rule_.value_decimal);
    ui->dSpinFinalTotal->setRange(DMIN, DMAX);
    ui->dSpinInitialTotal->setDecimals(section_rule_.value_decimal);
    ui->dSpinInitialTotal->setRange(DMIN, DMAX);
}

void InsertNodeOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_model_->ComboPathUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void InsertNodeOrder::accept()
{
    if (is_modified_) {
        if (!is_saved_) {
            emit QDialog::accepted();
            is_saved_ = true;
        }

        if (is_saved_)
            order_model_->UpdateNode(node_);

        is_modified_ = false;
    }
}

void InsertNodeOrder::reject()
{
    if (!is_saved_)
        ResourcePool<Node>::Instance().Recycle(node_);

    QDialog::reject();
}

void InsertNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &InsertNodeOrder::accept); }

void InsertNodeOrder::SetWidgetsDisabledBranch(bool disabled)
{
    ui->labelEmployee->setDisabled(disabled);
    ui->comboEmployee->setDisabled(disabled);

    ui->labelDiscount->setDisabled(disabled);
    ui->dSpinDiscount->setDisabled(disabled);

    ui->pBtnInsertParty->setDisabled(disabled);

    ui->dateTimeEdit->setDisabled(disabled);
    ui->chkBoxRefund->setDisabled(disabled);
    ui->labelInitialTotal->setDisabled(disabled);
    ui->dSpinInitialTotal->setDisabled(disabled);
    ui->pBtnPrint->setDisabled(disabled);
    ui->dSpinFinalTotal->setDisabled(disabled);
}

void InsertNodeOrder::SetWidgetsEnabledPost(bool enabled)
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

void InsertNodeOrder::ZeroSettlement()
{
    ui->dSpinDiscount->setValue(0.0);
    ui->dSpinFinalTotal->setValue(0.0);
}

void InsertNodeOrder::EnableSave(bool enable)
{
    is_modified_ = enable;
    ui->pBtnSaveOrder->setEnabled(enable);
}

void InsertNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;
    EnableSave(true);
}

void InsertNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
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

void InsertNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->node_rule = checked; }

void InsertNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    EnableSave(true);
}

void InsertNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;
    EnableSave(true);
}

void InsertNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ZeroSettlement();
    EnableSave(true);
}

void InsertNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ZeroSettlement();
    EnableSave(true);
}

void InsertNodeOrder::on_pBtnInsertParty_clicked()
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

void InsertNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(DATE_TIME_FST);
    EnableSave(true);
}

void InsertNodeOrder::on_pBtnLockOrder_toggled(bool checked)
{
    node_->locked = checked;
    SetWidgetsEnabledPost(!checked);
    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    if (checked) {
        accept();
    }
}

void InsertNodeOrder::on_lineDescription_textChanged(const QString& arg1)
{
    node_->description = arg1;
    EnableSave(true);
}

void InsertNodeOrder::on_dSpinDiscount_valueChanged(double arg1)
{
    node_->initial_total = node_->initial_total + node_->discount - arg1;
    ui->dSpinInitialTotal->setValue(node_->initial_total);
    node_->discount = arg1;
    EnableSave(true);
}

void InsertNodeOrder::on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1)
{
    node_->branch = { arg1 == Qt::Checked };
    SetWidgetsDisabledBranch(node_->branch);
}
