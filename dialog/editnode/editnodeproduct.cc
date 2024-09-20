#include "editnodeproduct.h"

#include "component/constvalue.h"
#include "ui_editnodeproduct.h"

EditNodeProduct::EditNodeProduct(
    Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool enable_branch, int ratio_decimal, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeProduct)
    , node_ { node }
    , parent_path_ { parent_path }
    , name_list_ { name_list }
{
    ui->setupUi(this);

    ui->comboUnit->blockSignals(true);

    IniDialog(unit_hash, ratio_decimal);
    IniConnect();
    Data(node, enable_branch);

    ui->comboUnit->blockSignals(false);
}

EditNodeProduct::~EditNodeProduct() { delete ui; }

void EditNodeProduct::IniDialog(CStringHash& unit_hash, int ratio_decimal)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::GetInputValidator());

    this->setWindowTitle(parent_path_ + node_->name);

    for (auto it = unit_hash.cbegin(); it != unit_hash.cend(); ++it)
        ui->comboUnit->addItem(it.value(), it.key());

    ui->comboUnit->model()->sort(0);

    ui->dSpinBoxUnitPrice->setRange(0.0, DMAX);
    ui->dSpinBoxCommission->setRange(0.0, DMAX);
    ui->dSpinBoxUnitPrice->setDecimals(ratio_decimal);
    ui->dSpinBoxCommission->setDecimals(ratio_decimal);
}

void EditNodeProduct::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeProduct::RNameEdited); }

void EditNodeProduct::Data(Node* node, bool enable_branch)
{
    int item_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(item_index);

    ui->rBtnDDCI->setChecked(node->node_rule);
    ui->rBtnDICD->setChecked(!node->node_rule);

    if (node->name.isEmpty())
        return ui->pBtnOk->setEnabled(false);

    ui->lineEditName->setText(node->name);
    ui->lineEditCode->setText(node->code);
    ui->lineEditDescription->setText(node->description);
    ui->plainTextEdit->setPlainText(node->note);
    ui->dSpinBoxUnitPrice->setValue(node->discount);
    ui->dSpinBoxCommission->setValue(node->second);

    ui->chkBoxBranch->setChecked(node->branch);
    ui->chkBoxBranch->setEnabled(enable_branch);
}

void EditNodeProduct::RNameEdited(const QString& arg1)
{
    auto simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void EditNodeProduct::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void EditNodeProduct::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void EditNodeProduct::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void EditNodeProduct::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void EditNodeProduct::on_rBtnDDCI_toggled(bool checked)
{
    if (node_->final_total != 0 && node_->node_rule != checked) {
        node_->final_total = -node_->final_total;
        node_->initial_total = -node_->initial_total;
    }
    node_->node_rule = checked;
}

void EditNodeProduct::on_chkBoxBranch_toggled(bool checked) { node_->branch = checked; }

void EditNodeProduct::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void EditNodeProduct::on_dSpinBoxUnitPrice_editingFinished() { node_->discount = ui->dSpinBoxUnitPrice->value(); }

void EditNodeProduct::on_dSpinBoxCommission_editingFinished() { node_->second = ui->dSpinBoxCommission->value(); }
