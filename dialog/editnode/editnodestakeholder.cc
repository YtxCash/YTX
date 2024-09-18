#include "editnodestakeholder.h"

#include <QTimer>

#include "component/constvalue.h"
#include "ui_editnodestakeholder.h"

EditNodeStakeholder::EditNodeStakeholder(Node* node, CSectionRule& section_rule, CString& separator, CInfo& info, bool node_usage, bool view_opened,
    int parent_id, CStringHash& term_hash, AbstractTreeModel* model, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeStakeholder)
    , node_ { node }
    , separator_ { separator }
    , term_hash_ { term_hash }
    , section_rule_ { section_rule }
    , info_ { info }
    , model_ { model }
    , parent_id_ { parent_id }
    , node_usage_ { node_usage }
    , view_opened_ { view_opened }
    , parent_path_ { model->GetPath(parent_id) }
{
    ui->setupUi(this);

    ui->comboUnit->blockSignals(true);
    ui->comboEmployee->blockSignals(true);
    ui->chkBoxBranch->blockSignals(true);

    IniDialog(info.unit_hash);
    IniConnect();
    Data(node);

    ui->comboUnit->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
    ui->chkBoxBranch->blockSignals(false);
}

EditNodeStakeholder::~EditNodeStakeholder() { delete ui; }

void EditNodeStakeholder::IniDialog(CStringHash& unit_hash)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::GetInputValidator());

    if (!parent_path_.isEmpty())
        parent_path_ += separator_;

    this->setWindowTitle(parent_path_ + node_->name);

    IniComboUnit(unit_hash);
    IniComboEmployee();

    ui->spinBoxPaymentPeriod->setRange(0, IMAX);
    ui->dSpinBoxTaxRate->setRange(0.0, DMAX);
    ui->dSpinBoxTaxRate->setDecimals(section_rule_.ratio_decimal);
}

void EditNodeStakeholder::IniComboUnit(CStringHash& unit_hash)
{
    ui->comboUnit->clear();

    for (auto it = unit_hash.cbegin(); it != unit_hash.cend(); ++it)
        ui->comboUnit->addItem(it.value(), it.key());

    ui->comboUnit->model()->sort(0);
}

void EditNodeStakeholder::IniComboEmployee()
{
    ui->comboEmployee->clear();

    model_->ComboPathUnit(ui->comboEmployee, 0);

    ui->comboEmployee->insertItem(0, QString(), 0);
    ui->comboEmployee->setCurrentIndex(0);
    ui->comboEmployee->model()->sort(0);
}

void EditNodeStakeholder::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeStakeholder::REditName); }

void EditNodeStakeholder::Data(Node* node)
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
    ui->chkBoxBranch->setEnabled(!node_usage_ && model_->ChildrenEmpty(node->id) && !view_opened_);
}

void EditNodeStakeholder::REditName(const QString& arg1)
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
