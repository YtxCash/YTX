#include "editnodestakeholder.h"

#include "component/constvalue.h"
#include "ui_editnodestakeholder.h"

EditNodeStakeholder::EditNodeStakeholder(Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool enable_branch,
    int ratio_decimal, AbstractTreeModel* model, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeStakeholder)
    , node_ { node }
    , parent_path_ { parent_path }
    , name_list_ { name_list }
{
    ui->setupUi(this);

    ui->comboUnit->blockSignals(true);
    ui->comboEmployee->blockSignals(true);
    ui->chkBoxBranch->blockSignals(true);

    IniDialog(unit_hash, model, ratio_decimal);
    IniConnect();
    Data(node, enable_branch);

    ui->comboUnit->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
    ui->chkBoxBranch->blockSignals(false);
}

EditNodeStakeholder::~EditNodeStakeholder() { delete ui; }

void EditNodeStakeholder::IniDialog(CStringHash& unit_hash, AbstractTreeModel* model, int ratio_decimal)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::GetInputValidator());

    this->setWindowTitle(parent_path_ + node_->name);

    IniComboWithStringHash(ui->comboUnit, unit_hash);
    IniComboEmployee(model);

    ui->spinBoxPaymentPeriod->setRange(IZERO, IMAX);
    ui->dSpinBoxTaxRate->setRange(0.0, DMAX);
    ui->dSpinBoxTaxRate->setDecimals(ratio_decimal);
}

void EditNodeStakeholder::IniComboWithStringHash(QComboBox* combo, CStringHash& hash)
{
    combo->clear();

    for (auto it = hash.cbegin(); it != hash.cend(); ++it)
        combo->addItem(it.value(), it.key());

    combo->model()->sort(0);
}

void EditNodeStakeholder::IniComboEmployee(AbstractTreeModel* model)
{
    ui->comboEmployee->clear();

    model->ComboPathUnit(ui->comboEmployee, 0);

    ui->comboEmployee->insertItem(0, QString(), 0);
    ui->comboEmployee->setCurrentIndex(0);
    ui->comboEmployee->model()->sort(0);
}

void EditNodeStakeholder::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeStakeholder::RNameEdited); }

void EditNodeStakeholder::Data(Node* node, bool enable_branch)
{
    int unit_index { ui->comboUnit->findData(node_->unit) };
    ui->comboUnit->setCurrentIndex(unit_index);

    ui->rBtnMonthly->setChecked(node->node_rule);
    ui->rBtnCash->setChecked(!node->node_rule);

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
    ui->spinBoxPaymentPeriod->setValue(node->first);
    ui->dSpinBoxTaxRate->setValue(node->second * HUNDRED);
    ui->spinBoxDeadline->setValue(node->party);

    ui->chkBoxBranch->setChecked(node->branch);
    ui->chkBoxBranch->setEnabled(enable_branch);
}

void EditNodeStakeholder::RNameEdited(const QString& arg1)
{
    auto simplified { arg1.simplified() };
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

void EditNodeStakeholder::on_spinBoxPaymentPeriod_editingFinished() { node_->first = ui->spinBoxPaymentPeriod->value(); }

void EditNodeStakeholder::on_spinBoxDeadline_editingFinished() { node_->party = ui->spinBoxDeadline->value(); }

void EditNodeStakeholder::on_dSpinBoxTaxRate_editingFinished() { node_->second = ui->dSpinBoxTaxRate->value() / HUNDRED; }

void EditNodeStakeholder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->employee = ui->comboEmployee->currentData().toInt();
}

void EditNodeStakeholder::on_rBtnMonthly_toggled(bool checked) { node_->node_rule = checked; }
