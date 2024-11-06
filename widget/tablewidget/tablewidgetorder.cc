#include "tablewidgetorder.h"

#include <QTimer>

#include "dialog/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_tablewidgetorder.h"

TableWidgetOrder::TableWidgetOrder(
    NodeShadow* node_shadow, Sqlite* sql, TableModel* order_table, TreeModel* stakeholder_tree, CSettings* settings, int party_unit, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetOrder)
    , node_shadow_ { node_shadow }
    , sql_ { sql }
    , party_unit_ { party_unit }
    , order_table_ { order_table }
    , stakeholder_tree_ { static_cast<TreeModelStakeholder*>(stakeholder_tree) }
    , settings_ { settings }
    , info_node_ { party_unit == UNIT_CUSTOMER ? SALES : PURCHASE }
    , node_id_ { *node_shadow->id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    bool locked { *node_shadow->locked };

    IniDialog();
    IniData();
    ui->tableViewOrder->setFocus();
    QTimer::singleShot(100, this, [this]() { IniDataCombo(); });

    IniUnit(*node_shadow_->unit);

    ui->chkBoxBranch->setEnabled(false);

    ui->pBtnLockOrder->setChecked(locked);
    ui->pBtnLockOrder->setText(locked ? tr("UnLock") : tr("Lock"));
    if (locked)
        ui->pBtnPrint->setFocus();

    LockWidgets(locked);
}

TableWidgetOrder::~TableWidgetOrder()
{
    ResourcePool<NodeShadow>::Instance().Recycle(node_shadow_);
    delete ui;
}

QPointer<QTableView> TableWidgetOrder::View() const { return ui->tableViewOrder; }

void TableWidgetOrder::RUpdateComboModel()
{
    if (*node_shadow_->branch)
        return;

    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_party_, party_unit_, UnitFilterMode::kIncludeUnitOnly);
    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_employee_, UNIT_EMPLOYEE, UnitFilterMode::kIncludeUnitOnlyWithEmpty);

    int index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    int index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

void TableWidgetOrder::RUpdateData(int node_id, TreeEnumOrder column, const QVariant& value)
{
    if (node_id != node_id_)
        return;

    SignalBlocker blocker(this);

    switch (column) {
    case TreeEnumOrder::kDescription:
        ui->lineDescription->setText(value.toString());
        break;
    case TreeEnumOrder::kRule:
        ui->chkBoxRefund->setChecked(value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        IniUnit(value.toInt());
        break;
    case TreeEnumOrder::kParty: {
        int party_index { ui->comboParty->findData(value.toInt()) };
        ui->comboParty->setCurrentIndex(party_index);
        break;
    }
    case TreeEnumOrder::kEmployee: {
        int employee_index { ui->comboEmployee->findData(value.toInt()) };
        ui->comboEmployee->setCurrentIndex(employee_index);
        break;
    }
    case TreeEnumOrder::kDateTime:
        ui->dateTimeEdit->setDateTime(QDateTime::fromString(value.toString(), DATE_TIME_FST));
        break;
    case TreeEnumOrder::kLocked: {
        bool locked { value.toBool() };

        ui->pBtnLockOrder->setChecked(locked);
        ui->pBtnLockOrder->setText(locked ? tr("UnLock") : tr("Lock"));
        LockWidgets(locked);

        if (locked) {
            ui->pBtnPrint->setFocus();
            ui->pBtnPrint->setDefault(true);
            ui->tableViewOrder->clearSelection();
        }
        break;
    }
    default:
        return;
    }
}

void TableWidgetOrder::RUpdateLeafValueTO(int /*node_id*/, double diff) { ui->dSpinFirst->setValue(ui->dSpinFirst->value() + diff); }

void TableWidgetOrder::RUpdateLeafValueFPTO(
    int /*node_id*/, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    ui->dSpinFirst->setValue(ui->dSpinFirst->value() + first_diff);
    ui->dSpinSecond->setValue(ui->dSpinSecond->value() + second_diff);
    ui->dSpinAmount->setValue(ui->dSpinAmount->value() + amount_diff);
    ui->dSpinDiscount->setValue(ui->dSpinDiscount->value() + discount_diff);
    ui->dSpinSettled->setValue(ui->dSpinSettled->value() + (*node_shadow_->unit == UNIT_CASH ? settled_diff : 0.0));
}

void TableWidgetOrder::IniDialog()
{
    combo_model_party_ = new QStandardItemModel(this);
    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_party_, party_unit_, UnitFilterMode::kIncludeUnitOnly);
    ui->comboParty->setModel(combo_model_party_);
    ui->comboParty->setCurrentIndex(-1);

    combo_model_employee_ = new QStandardItemModel(this);
    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_employee_, UNIT_EMPLOYEE, UnitFilterMode::kIncludeUnitOnlyWithEmpty);
    ui->comboEmployee->setModel(combo_model_employee_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(DATE_TIME_FST);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinAmount->setRange(DMIN, DMAX);
    ui->dSpinSettled->setRange(DMIN, DMAX);
    ui->dSpinSecond->setRange(DMIN, DMAX);
    ui->dSpinFirst->setRange(DMIN, DMAX);

    ui->dSpinDiscount->setDecimals(settings_->amount_decimal);
    ui->dSpinAmount->setDecimals(settings_->amount_decimal);
    ui->dSpinSettled->setDecimals(settings_->amount_decimal);
    ui->dSpinSecond->setDecimals(settings_->common_decimal);
    ui->dSpinFirst->setDecimals(settings_->common_decimal);
}

void TableWidgetOrder::IniData()
{
    ui->dSpinSettled->setValue(*node_shadow_->final_total);
    ui->dSpinDiscount->setValue(*node_shadow_->discount);
    ui->dSpinFirst->setValue(*node_shadow_->first);
    ui->dSpinSecond->setValue(*node_shadow_->second);
    ui->dSpinAmount->setValue(*node_shadow_->initial_total);

    ui->chkBoxRefund->setChecked(*node_shadow_->rule);
    ui->chkBoxBranch->setChecked(*node_shadow_->branch);
    ui->lineDescription->setText(*node_shadow_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(*node_shadow_->date_time, DATE_TIME_FST));
    ui->pBtnLockOrder->setChecked(*node_shadow_->locked);

    ui->tableViewOrder->setModel(order_table_);
}

void TableWidgetOrder::IniDataCombo()
{
    int party_index { ui->comboParty->findData(*node_shadow_->party) };
    ui->comboParty->setCurrentIndex(party_index);

    int employee_index { ui->comboEmployee->findData(*node_shadow_->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);
}

void TableWidgetOrder::LockWidgets(bool locked)
{
    const bool enable { !locked };

    ui->labelParty->setEnabled(enable);
    ui->comboParty->setEnabled(enable);

    ui->pBtnInsertParty->setEnabled(enable);

    ui->labelSettled->setEnabled(enable);
    ui->dSpinSettled->setEnabled(enable);

    ui->dSpinAmount->setEnabled(enable);

    ui->labelDiscount->setEnabled(enable);
    ui->dSpinDiscount->setEnabled(enable);

    ui->labelEmployee->setEnabled(enable);
    ui->comboEmployee->setEnabled(enable);
    ui->tableViewOrder->setEnabled(enable);

    ui->rBtnCash->setEnabled(enable);
    ui->rBtnMonthly->setEnabled(enable);
    ui->rBtnPending->setEnabled(enable);
    ui->dateTimeEdit->setEnabled(enable);

    ui->dSpinFirst->setEnabled(enable);
    ui->labelFirst->setEnabled(enable);
    ui->dSpinSecond->setEnabled(enable);
    ui->labelSecond->setEnabled(enable);

    ui->chkBoxRefund->setEnabled(enable);
    ui->lineDescription->setEnabled(enable);

    ui->pBtnPrint->setEnabled(locked);
}

void TableWidgetOrder::IniUnit(int unit)
{
    switch (unit) {
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
}

void TableWidgetOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!*node_shadow_->branch || arg1.isEmpty())
        return;

    *node_shadow_->name = arg1;
    sql_->UpdateField(info_node_, arg1, NAME, node_id_);
}

void TableWidgetOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (*node_shadow_->branch)
        return;

    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    *node_shadow_->party = party_id;
    emit SUpdateParty();
    sql_->UpdateField(info_node_, party_id, PARTY, node_id_);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == RULE_CASH);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == RULE_MONTHLY);
}

void TableWidgetOrder::on_chkBoxRefund_toggled(bool checked)
{
    *node_shadow_->rule = checked;
    sql_->UpdateField(info_node_, checked, RULE, node_id_);
}

void TableWidgetOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    *node_shadow_->employee = ui->comboEmployee->currentData().toInt();
    sql_->UpdateField(info_node_, *node_shadow_->employee, EMPLOYEE, node_id_);
}

void TableWidgetOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_CASH;

    *node_shadow_->final_total = *node_shadow_->initial_total - *node_shadow_->discount;
    ui->dSpinSettled->setValue(*node_shadow_->final_total);

    sql_->UpdateField(info_node_, UNIT_CASH, UNIT, node_id_);
    sql_->UpdateField(info_node_, *node_shadow_->final_total, SETTLED, node_id_);
}

void TableWidgetOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_MONTHLY;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    sql_->UpdateField(info_node_, UNIT_MONTHLY, UNIT, node_id_);
    sql_->UpdateField(info_node_, 0.0, SETTLED, node_id_);
}

void TableWidgetOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_PENDING;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    sql_->UpdateField(info_node_, UNIT_PENDING, UNIT, node_id_);
    sql_->UpdateField(info_node_, 0.0, SETTLED, node_id_);
}

void TableWidgetOrder::on_pBtnInsertParty_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (*node_shadow_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = stakeholder_tree_->Rule(-1);
    stakeholder_tree_->SetParent(node, -1);
    node->name = name;

    node->unit = party_unit_;

    stakeholder_tree_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void TableWidgetOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    *node_shadow_->date_time = date_time.toString(DATE_TIME_FST);
    sql_->UpdateField(info_node_, *node_shadow_->date_time, DATE_TIME, node_id_);
}

void TableWidgetOrder::on_lineDescription_editingFinished()
{
    *node_shadow_->description = ui->lineDescription->text();
    sql_->UpdateField(info_node_, *node_shadow_->description, DESCRIPTION, node_id_);
}

void TableWidgetOrder::on_pBtnLockOrder_toggled(bool checked)
{
    *node_shadow_->locked = checked;
    sql_->UpdateField(info_node_, checked, LOCKED, node_id_);
    emit SUpdateLocked(node_id_, checked);

    ui->pBtnLockOrder->setText(checked ? tr("UnLock") : tr("Lock"));

    LockWidgets(checked);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
        ui->tableViewOrder->clearSelection();
    }
}
