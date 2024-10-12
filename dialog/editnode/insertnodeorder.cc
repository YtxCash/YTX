#include "insertnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "dialog/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_insertnodeorder.h"

InsertNodeOrder::InsertNodeOrder(
    Node* node, SPSqlite sql, TableModel* order_table, TreeModel* stakeholder_model, CSettings& settings, int unit_party, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeOrder)
    , sql_ { sql }
    , node_ { node }
    , unit_party_ { unit_party }
    , order_table_ { order_table }
    , stakeholder_model_ { stakeholder_model }
    , settings_ { settings }
    , info_node_ { unit_party == UNIT_CUSTOMER ? SALES : PURCHASE }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog();
    IniConnect();

    ui->tableViewOrder->setModel(order_table);
    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnLockOrder->setEnabled(false);

    ui->pBtnLockOrder->setText(tr("Lock"));
    ui->labelParty->setText(tr("Party"));
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

    stakeholder_model_->LeafPathSpecificUnit(ui->comboEmployee, UNIT_EMPLOYEE);
    stakeholder_model_->LeafPathSpecificUnit(ui->comboParty, unit_party_);

    ui->comboEmployee->model()->sort(0);
    ui->comboParty->model()->sort(0);

    auto index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    auto index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

void InsertNodeOrder::RUpdateLocked(int node_id, bool checked)
{
    if (node_id != node_id_)
        return;

    ui->pBtnLockOrder->blockSignals(true);
    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));
    ui->pBtnLockOrder->setChecked(checked);
    ui->pBtnLockOrder->blockSignals(false);

    LockWidgets(checked, node_->branch);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
        ui->tableViewOrder->clearSelection();
    }
}

QTableView* InsertNodeOrder::View() { return ui->tableViewOrder; }

void InsertNodeOrder::IniDialog()
{
    IniCombo(ui->comboParty, unit_party_);
    IniCombo(ui->comboEmployee, UNIT_EMPLOYEE);

    ui->dateTimeEdit->setDisplayFormat(DATE_TIME_FST);
    ui->comboParty->lineEdit()->setValidator(&LineEdit::GetInputValidator());

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinAmount->setRange(DMIN, DMAX);
    ui->dSpinSettled->setRange(DMIN, DMAX);
    ui->dSpinSecond->setRange(DMIN, DMAX);
    ui->dSpinFirst->setRange(DMIN, DMAX);

    ui->dSpinDiscount->setDecimals(settings_.amount_decimal);
    ui->dSpinAmount->setDecimals(settings_.amount_decimal);
    ui->dSpinSettled->setDecimals(settings_.amount_decimal);
    ui->dSpinSecond->setDecimals(settings_.common_decimal);
    ui->dSpinFirst->setDecimals(settings_.common_decimal);

    ui->comboParty->setFocus();
}

void InsertNodeOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_model_->LeafPathSpecificUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void InsertNodeOrder::accept()
{
    if (auto focus_widget { this->focusWidget() })
        focus_widget->clearFocus();

    if (!is_saved_) {
        emit QDialog::accepted();
        is_saved_ = true;
        node_id_ = node_->id;
        emit SUpdateNodeID(node_id_);
        ui->chkBoxBranch->setEnabled(false);
        ui->pBtnSaveOrder->setEnabled(false);
        ui->tableViewOrder->clearSelection();
    }
}

void InsertNodeOrder::reject()
{
    if (!is_saved_)
        ResourcePool<Node>::Instance().Recycle(node_);

    QDialog::reject();
}

void InsertNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &InsertNodeOrder::accept); }

void InsertNodeOrder::LockWidgets(bool locked, bool branch)
{
    bool basic_enable { !locked };
    bool not_branch_enable { !locked && !branch };

    ui->labelParty->setEnabled(basic_enable);
    ui->comboParty->setEnabled(basic_enable);

    ui->pBtnInsertParty->setEnabled(not_branch_enable);

    ui->labelSettled->setEnabled(not_branch_enable);
    ui->dSpinSettled->setEnabled(not_branch_enable);

    ui->dSpinAmount->setEnabled(!branch);

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

void InsertNodeOrder::UpdateUnit(int unit)
{
    switch (unit) {
    case UNIT_CASH:
        ui->rBtnCash->setChecked(true);
        ui->dSpinAmount->setValue(ui->dSpinSettled->value() - ui->dSpinDiscount->value());
        break;
    case UNIT_MONTHLY:
        ui->dSpinAmount->setValue(0.0);
        ui->rBtnMonthly->setChecked(true);
        break;
    case UNIT_PENDING:
        ui->dSpinAmount->setValue(0.0);
        ui->rBtnPending->setChecked(true);
        break;
    default:
        break;
    }
}

void InsertNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;

    if (!is_saved_) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnLockOrder->setEnabled(true);
    }

    if (is_saved_)
        sql_->UpdateField(info_node_, arg1, NAME, node_id_);
}

void InsertNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (node_->branch)
        return;

    auto party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    node_->party = party_id;
    if (!is_saved_) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnLockOrder->setEnabled(true);
    }

    if (is_saved_)
        sql_->UpdateField(info_node_, party_id, PARTY, node_id_);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    auto employee_index { ui->comboEmployee->findData(stakeholder_model_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_model_->Rule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakeholder_model_->Rule(party_id) == 1);
}

void InsertNodeOrder::on_chkBoxRefund_toggled(bool checked)
{
    node_->rule = checked;

    if (is_saved_)
        sql_->UpdateField(info_node_, checked, RULE, node_id_);
}

void InsertNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    if (!is_saved_) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnLockOrder->setEnabled(true);
    }

    if (is_saved_)
        sql_->UpdateField(info_node_, node_->employee, EMPLOYEE, node_id_);
}

void InsertNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;

    ui->dSpinAmount->setValue(ui->dSpinSettled->value() - ui->dSpinDiscount->value());
    node_->final_total = ui->dSpinAmount->value();

    if (is_saved_) {
        sql_->UpdateField(info_node_, UNIT_CASH, UNIT, node_id_);
        sql_->UpdateField(info_node_, node_->final_total, FINAL_TOTAL, node_id_);
    }
}

void InsertNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ui->dSpinAmount->setValue(0.0);
    node_->final_total = 0.0;

    if (is_saved_) {
        sql_->UpdateField(info_node_, UNIT_MONTHLY, UNIT, node_id_);
        sql_->UpdateField(info_node_, 0.0, FINAL_TOTAL, node_id_);
    }
}

void InsertNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ui->dSpinAmount->setValue(0.0);
    node_->final_total = 0.0;

    if (is_saved_) {
        sql_->UpdateField(info_node_, UNIT_PENDING, UNIT, node_id_);
        sql_->UpdateField(info_node_, 0.0, FINAL_TOTAL, node_id_);
    }
}

void InsertNodeOrder::on_pBtnInsertParty_clicked()
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

void InsertNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(DATE_TIME_FST);

    if (is_saved_)
        sql_->UpdateField(info_node_, node_->date_time, DATE_TIME, node_id_);
}

void InsertNodeOrder::on_pBtnLockOrder_toggled(bool checked)
{
    node_->locked = checked;

    if (is_saved_) {
        sql_->UpdateField(info_node_, checked, LOCKED, node_id_);
        emit SUpdateLocked(node_id_, checked);
    }

    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    LockWidgets(checked, node_->branch);

    if (checked) {
        accept();

        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void InsertNodeOrder::on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1)
{
    bool enable { arg1 == Qt::Checked };
    node_->branch = enable;
    LockWidgets(false, enable);
}

void InsertNodeOrder::on_lineDescription_editingFinished()
{
    node_->description = ui->lineDescription->text();

    if (is_saved_)
        sql_->UpdateField(info_node_, node_->description, DESCRIPTION, node_id_);
}
