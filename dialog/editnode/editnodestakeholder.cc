#include "editnodestakeholder.h"

#include "component/constvalue.h"
#include "dialog/signalblocker.h"
#include "ui_editnodestakeholder.h"

EditNodeStakeholder::EditNodeStakeholder(Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool branch_enable, bool unit_enable,
    int amount_decimal, TreeModel* stakeholder_tree, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeStakeholder)
    , node_ { node }
    , parent_path_ { parent_path }
    , name_list_ { name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(unit_hash, stakeholder_tree, amount_decimal);
    IniConnect();
    Data(node, branch_enable, unit_enable);
}

EditNodeStakeholder::~EditNodeStakeholder() { delete ui; }

void EditNodeStakeholder::IniDialog(CStringHash& unit_hash, TreeModel* stakeholder_tree, int amount_decimal)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_ + node_->name);

    IniComboWithStringHash(ui->comboUnit, unit_hash);
    IniComboEmployee(stakeholder_tree);

    ui->dSpinPaymentPeriod->setRange(IZERO, IMAX);
    ui->dSpinTaxRate->setRange(0.0, DMAX);
    ui->dSpinTaxRate->setDecimals(amount_decimal);

    ui->deadline->setDateTime(QDateTime::currentDateTime());
    ui->deadline->setDisplayFormat(DD);
    ui->deadline->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
}

void EditNodeStakeholder::IniComboWithStringHash(QComboBox* combo, CStringHash& hash)
{
    combo->clear();

    for (auto it = hash.cbegin(); it != hash.cend(); ++it)
        combo->addItem(it.value(), it.key());

    combo->model()->sort(0);
}

void EditNodeStakeholder::IniComboEmployee(TreeModel* stakeholder_tree)
{
    ui->comboEmployee->clear();

    // 不需要接收更新combo model的信号
    auto* combo_model_ { new QStandardItemModel(this) };
    stakeholder_tree->LeafPathSpecificUnit(combo_model_, UNIT_EMPLOYEE, UnitFilterMode::kIncludeUnitOnly);
    ui->comboEmployee->setModel(combo_model_);

    ui->comboEmployee->insertItem(0, QString(), 0);
    ui->comboEmployee->setCurrentIndex(0);
    ui->comboEmployee->model()->sort(0);
}

void EditNodeStakeholder::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeStakeholder::RNameEdited); }

void EditNodeStakeholder::Data(Node* node, bool branch_enable, bool unit_enable)
{
    int unit_index { ui->comboUnit->findData(node_->unit) };
    ui->comboUnit->setCurrentIndex(unit_index);
    ui->comboUnit->setEnabled(unit_enable);

    ui->rBtnMonthly->setChecked(node->rule);
    ui->rBtnCash->setChecked(!node->rule);

    if (node->name.isEmpty()) {
        ui->pBtnOk->setEnabled(false);
        return;
    }

    int employee_index { ui->comboEmployee->findData(node->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->lineEditName->setText(node->name);
    ui->lineEditCode->setText(node->code);
    ui->lineEditDescription->setText(node->description);
    ui->plainTextEdit->setPlainText(node->note);
    ui->dSpinPaymentPeriod->setValue(node->first);
    ui->dSpinTaxRate->setValue(node->second * HUNDRED);
    ui->deadline->setDateTime(QDateTime::fromString(node->date_time, DATE_TIME_FST));

    ui->chkBoxBranch->setChecked(node->branch);
    ui->chkBoxBranch->setEnabled(branch_enable);
}

void EditNodeStakeholder::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void EditNodeStakeholder::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void EditNodeStakeholder::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void EditNodeStakeholder::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void EditNodeStakeholder::on_chkBoxBranch_toggled(bool checked) { node_->branch = checked; }

void EditNodeStakeholder::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void EditNodeStakeholder::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void EditNodeStakeholder::on_dSpinPaymentPeriod_editingFinished() { node_->first = ui->dSpinPaymentPeriod->value(); }

void EditNodeStakeholder::on_dSpinTaxRate_editingFinished() { node_->second = ui->dSpinTaxRate->value() / HUNDRED; }

void EditNodeStakeholder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->employee = ui->comboEmployee->currentData().toInt();
}

void EditNodeStakeholder::on_rBtnMonthly_toggled(bool checked) { node_->rule = checked; }

void EditNodeStakeholder::on_deadline_editingFinished() { node_->date_time = ui->deadline->dateTime().toString(DATE_TIME_FST); }
