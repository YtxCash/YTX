#include "editnodestakeholder.h"

#include <QTimer>

#include "component/constvalue.h"
#include "ui_editnodestakeholder.h"

EditNodeStakeholder::EditNodeStakeholder(Node* node, CSectionRule& section_rule, CString& separator, CInfo& info, bool node_usage, bool view_opened,
    int parent_id, CStringHash& term_hash, TreeModel* model, QWidget* parent)
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

    ui->comboMark->blockSignals(true);
    ui->comboEmployee->blockSignals(true);
    ui->chkBoxBranch->blockSignals(true);

    IniDialog(info.unit_hash);
    IniConnect();
    Data(node);

    ui->comboMark->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
    ui->chkBoxBranch->blockSignals(false);
}

EditNodeStakeholder::~EditNodeStakeholder() { delete ui; }

void EditNodeStakeholder::IniDialog(CStringHash& unit_hash)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(new QRegularExpressionValidator(QRegularExpression("[\\p{L} ()（）\\d]*"), this));

    if (!parent_path_.isEmpty())
        parent_path_ += separator_;

    this->setWindowTitle(parent_path_ + node_->name);

    IniComboMark(node_->branch, unit_hash);

    IniComboEmployee();

    ui->spinBoxPaymentPeriod->setRange(0, IMAX);
    ui->dateEditDeadline->setDisplayFormat(DATE_D);
    ui->dSpinBoxTaxRate->setRange(0.0, DMAX);
    ui->dSpinBoxTaxRate->setDecimals(section_rule_.ratio_decimal);
}

void EditNodeStakeholder::IniComboMark(bool branch, CStringHash& mark_hash)
{
    ui->comboMark->clear();

    for (auto it = mark_hash.cbegin(); it != mark_hash.cend(); ++it) {
        if ((branch && it.key() != 3) || (!branch && it.key() == 3))
            ui->comboMark->addItem(it.value(), it.key());
    }

    if (branch)
        ui->comboMark->model()->sort(0);
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
    int mark_index { ui->comboMark->findData(node_->unit) };
    ui->comboMark->setCurrentIndex(mark_index);

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
    ui->dateEditDeadline->setDateTime(QDateTime::fromString(node->date_time, DATE_D));

    ui->chkBoxBranch->setChecked(node->branch);
    ui->chkBoxBranch->setEnabled(!node_usage_ && node->children.isEmpty() && !view_opened_);
}

void EditNodeStakeholder::EnableEmployee(bool employee)
{
    ui->comboEmployee->setEnabled(employee);
    ui->labelEmployee->setEnabled(employee);
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

void EditNodeStakeholder::on_chkBoxBranch_toggled(bool checked)
{
    node_->branch = checked;
    IniComboMark(checked, info_.unit_hash);
}

void EditNodeStakeholder::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void EditNodeStakeholder::on_comboMark_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboMark->currentData().toInt();
    EnableEmployee(index == 1);
}

void EditNodeStakeholder::on_spinBoxPaymentPeriod_editingFinished() { node_->first = ui->spinBoxPaymentPeriod->value(); }

void EditNodeStakeholder::on_dateEditDeadline_editingFinished() { node_->date_time = ui->dateEditDeadline->dateTime().toString(DATE_D); }

void EditNodeStakeholder::on_dSpinBoxTaxRate_editingFinished() { node_->second = ui->dSpinBoxTaxRate->value() / HUNDRED; }

void EditNodeStakeholder::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ActivationChange && this->isActiveWindow()) {
        QTimer::singleShot(100, this, [&, this]() {
            name_list_.clear();
            model_->ChildrenName(name_list_, parent_id_, node_->id);
            REditName(ui->lineEditName->text());
            IniComboEmployee();
        });
    }

    QDialog::changeEvent(event);
}

void EditNodeStakeholder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->employee = ui->comboEmployee->currentData().toInt();
}

void EditNodeStakeholder::on_rBtnMonthly_toggled(bool checked) { node_->node_rule = checked; }
