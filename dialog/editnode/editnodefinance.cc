#include "editnodefinance.h"

#include "ui_editnodefinance.h"

EditNodeFinance::EditNodeFinance(Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool enable_branch, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeFinance)
    , node_ { node }
    , parent_path_ { parent_path }
    , name_list_ { name_list }
{
    ui->setupUi(this);

    ui->comboUnit->blockSignals(true);

    IniDialog(unit_hash);
    IniConnect();
    Data(node, enable_branch);

    ui->comboUnit->blockSignals(false);
}

EditNodeFinance::~EditNodeFinance() { delete ui; }

void EditNodeFinance::IniDialog(CStringHash& unit_hash)
{
    ui->lineName->setFocus();
    ui->lineName->setValidator(&LineEdit::GetInputValidator());

    this->setWindowTitle(parent_path_ + node_->name);

    for (auto it = unit_hash.cbegin(); it != unit_hash.cend(); ++it)
        ui->comboUnit->addItem(it.value(), it.key());

    ui->comboUnit->model()->sort(0);
}

void EditNodeFinance::IniConnect() { connect(ui->lineName, &QLineEdit::textEdited, this, &EditNodeFinance::RNameEdited); }

void EditNodeFinance::Data(Node* node, bool enable_branch)
{
    int item_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(item_index);

    ui->rBtnDDCI->setChecked(node->node_rule);
    ui->rBtnDICD->setChecked(!node->node_rule);

    if (node->name.isEmpty())
        return ui->pBtnOk->setEnabled(false);

    ui->lineName->setText(node->name);
    ui->lineCode->setText(node->code);
    ui->lineDescription->setText(node->description);
    ui->plainNote->setPlainText(node->note);

    ui->chkBoxBranch->setChecked(node->branch);
    ui->chkBoxBranch->setEnabled(enable_branch);
}

void EditNodeFinance::RNameEdited(const QString& arg1)
{
    auto simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void EditNodeFinance::on_lineName_editingFinished() { node_->name = ui->lineName->text(); }

void EditNodeFinance::on_lineCode_editingFinished() { node_->code = ui->lineCode->text(); }

void EditNodeFinance::on_lineDescription_editingFinished() { node_->description = ui->lineDescription->text(); }

void EditNodeFinance::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void EditNodeFinance::on_rBtnDDCI_toggled(bool checked)
{
    if (node_->final_total != 0 && node_->node_rule != checked) {
        node_->final_total = -node_->final_total;
        node_->initial_total = -node_->initial_total;
    }
    node_->node_rule = checked;
}

void EditNodeFinance::on_chkBoxBranch_toggled(bool checked) { node_->branch = checked; }

void EditNodeFinance::on_plainNote_textChanged() { node_->note = ui->plainNote->toPlainText(); }
