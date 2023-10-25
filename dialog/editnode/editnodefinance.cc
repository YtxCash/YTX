#include "editnodefinance.h"

#include <QTimer>

#include "ui_editnodefinance.h"

EditNodeFinance::EditNodeFinance(
    Node* node, CString& separator, CInfo& info, const TreeModel& model, int parent_id, bool node_usage, bool view_opened, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeFinance)
    , node_ { node }
    , separator_ { separator }
    , model_ { model }
    , parent_id_ { parent_id }
    , node_usage_ { node_usage }
    , view_opened_ { view_opened }
    , parent_path_ { model.GetPath(parent_id) }
{
    ui->setupUi(this);

    ui->comboUnit->blockSignals(true);

    IniDialog(info.unit_hash);
    IniConnect();
    Data(node);

    ui->comboUnit->blockSignals(false);

    if (info.section == Section::kFinance)
        ui->labUnit->setText(tr("Currency"));
}

EditNodeFinance::~EditNodeFinance() { delete ui; }

void EditNodeFinance::IniDialog(CStringHash& unit_hash)
{
    ui->lineName->setFocus();
    ui->lineName->setValidator(new QRegularExpressionValidator(QRegularExpression("[\\p{L} ()（）\\d]*"), this));

    if (!parent_path_.isEmpty())
        parent_path_ += separator_;

    this->setWindowTitle(parent_path_ + node_->name);

    for (auto it = unit_hash.cbegin(); it != unit_hash.cend(); ++it)
        ui->comboUnit->addItem(it.value(), it.key());

    ui->comboUnit->model()->sort(0);
}

void EditNodeFinance::IniConnect() { connect(ui->lineName, &QLineEdit::textEdited, this, &EditNodeFinance::REditName); }

void EditNodeFinance::Data(Node* node)
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
    ui->chkBoxBranch->setEnabled(!node_usage_ && node->children.isEmpty() && !view_opened_);
}

void EditNodeFinance::REditName(const QString& arg1)
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

void EditNodeFinance::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ActivationChange && this->isActiveWindow()) {
        QTimer::singleShot(100, this, [&, this]() {
            name_list_.clear();
            model_.ChildrenName(name_list_, parent_id_, node_->id);
            REditName(ui->lineName->text());
        });
    }

    QDialog::changeEvent(event);
}
