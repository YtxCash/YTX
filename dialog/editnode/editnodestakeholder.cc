#include "editnodestakeholder.h"

#include <QTimer>

#include "component/constvalue.h"
#include "ui_editnodestakeholder.h"

EditNodeStakeholder::EditNodeStakeholder(Node* node, const SectionRule* section_rule, CString* separator, const Info* info, bool node_usage, bool view_opened,
    CString& parent_path, CStringHash* node_term_hash, AbstractTreeModel* model, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeStakeholder)
    , node_ { node }
    , separator_ { separator }
    , node_term_hash_ { node_term_hash }
    , branch_path_ { model->BranchPath() }
    , section_rule_ { section_rule }
    , node_hash_ { model->GetNodeHash() }
    , info_ { info }
    , node_usage_ { node_usage }
    , view_opened_ { view_opened }
    , parent_path_ { parent_path }
{
    ui->setupUi(this);

    ui->comboMark->blockSignals(true);
    ui->comboTerm->blockSignals(true);
    ui->comboEmployee->blockSignals(true);
    ui->chkBoxBranch->blockSignals(true);

    IniDialog(&info->unit_hash);
    IniConnect();
    Data(node);

    ui->comboMark->blockSignals(false);
    ui->comboTerm->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
    ui->chkBoxBranch->blockSignals(false);
}

EditNodeStakeholder::~EditNodeStakeholder() { delete ui; }

void EditNodeStakeholder::IniDialog(CStringHash* unit_hash)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(new QRegularExpressionValidator(QRegularExpression("[\\p{L} ()（）\\d]*"), this));

    if (!parent_path_.isEmpty())
        parent_path_ += *separator_;

    this->setWindowTitle(parent_path_ + node_->name);

    IniComboMark(node_->branch, unit_hash);

    for (auto it = node_term_hash_->cbegin(); it != node_term_hash_->cend(); ++it)
        ui->comboTerm->addItem(it.value(), it.key());

    ui->comboTerm->model()->sort(0);

    IniComboEmployee();

    ui->spinBoxPaymentPeriod->setRange(0, IMAX);
    ui->dateEditDeadline->setDisplayFormat(DATE_D);
    ui->dSpinBoxTaxRate->setRange(0.0, DMAX);
    ui->dSpinBoxTaxRate->setDecimals(section_rule_->ratio_decimal);
}

void EditNodeStakeholder::IniComboMark(bool branch, CStringHash* unit_hash)
{
    ui->comboMark->clear();

    for (auto it = unit_hash->cbegin(); it != unit_hash->cend(); ++it) {
        if ((branch && it.key() != 3) || (!branch && it.key() == 3))
            ui->comboMark->addItem(it.value(), it.key());
    }

    if (branch)
        ui->comboMark->model()->sort(0);
}

void EditNodeStakeholder::IniComboEmployee()
{
    ui->comboEmployee->clear();

    for (auto it = branch_path_->cbegin(); it != branch_path_->cend(); ++it)
        if (node_hash_->value(it.key())->unit == 0)
            ui->comboEmployee->addItem(it.value(), it.key());

    ui->comboEmployee->insertItem(0, QString(), 0);
    ui->comboEmployee->setCurrentIndex(0);
    ui->comboEmployee->model()->sort(0);
}

void EditNodeStakeholder::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeStakeholder::REditName); }

void EditNodeStakeholder::Data(Node* node)
{
    int mark_index { ui->comboMark->findData(node_->unit) };
    ui->comboMark->setCurrentIndex(mark_index);

    int term_index { ui->comboTerm->findData(node->node_rule) };
    ui->comboTerm->setCurrentIndex(term_index);

    ui->chkBoxBranch->setChecked(node->branch);

    bool editable { parent_path_.isEmpty() && node->name.isEmpty() };
    ui->comboMark->setEnabled(editable);
    ui->labMark->setEnabled(editable);

    if (node->name.isEmpty()) {
        ui->pBtnOk->setEnabled(false);
        return;
    }

    int employee_index { ui->comboEmployee->findData(node->second_property) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->lineEditName->setText(node->name);
    ui->lineEditCode->setText(node->code);
    ui->lineEditDescription->setText(node->description);
    ui->plainTextEdit->setPlainText(node->note);
    ui->spinBoxPaymentPeriod->setValue(node->first_property);
    ui->dSpinBoxTaxRate->setValue(node->third_property * HUNDRED);
    ui->dateEditDeadline->setDateTime(QDateTime::fromString(node->date_time, DATE_D));

    ui->chkBoxBranch->setEnabled(!node_usage_ && node->children.isEmpty() && !view_opened_);
}

void EditNodeStakeholder::REditName(const QString& arg1)
{
    auto simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !node_name_list_.contains(simplified));
}

void EditNodeStakeholder::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void EditNodeStakeholder::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void EditNodeStakeholder::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void EditNodeStakeholder::on_chkBoxBranch_toggled(bool checked)
{
    node_->branch = checked;
    IniComboMark(checked, &info_->unit_hash);
}

void EditNodeStakeholder::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void EditNodeStakeholder::on_comboMark_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboMark->currentData().toInt();
}

void EditNodeStakeholder::on_comboTerm_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->node_rule = ui->comboTerm->currentData().toBool();
}

void EditNodeStakeholder::on_spinBoxPaymentPeriod_editingFinished() { node_->first_property = ui->spinBoxPaymentPeriod->value(); }

void EditNodeStakeholder::on_dateEditDeadline_editingFinished() { node_->date_time = ui->dateEditDeadline->dateTime().toString(DATE_D); }

void EditNodeStakeholder::on_dSpinBoxTaxRate_editingFinished() { node_->third_property = ui->dSpinBoxTaxRate->value() / HUNDRED; }

void EditNodeStakeholder::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ActivationChange && this->isActiveWindow()) {
        QTimer::singleShot(100, this, [&, this]() {
            node_name_list_.clear();
            for (const auto& node : node_->parent->children)
                if (node->id != node_->id)
                    node_name_list_.emplaceBack(node->name);

            REditName(ui->lineEditName->text());
            IniComboEmployee();
        });
    }

    QDialog::changeEvent(event);
}

void EditNodeStakeholder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->second_property = ui->comboEmployee->currentData().toInt();
}
