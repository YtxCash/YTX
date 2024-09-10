#include "editnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "component/data.h"
#include "global/resourcepool.h"
#include "ui_editnodeorder.h"

EditNodeOrder::EditNodeOrder(Node* node, CSectionRule& section_rule, AbstractTreeModel* order_model, Tree* stakeholder, const AbstractTreeModel& product_model,
    CInfo& info, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeOrder)
    , node_ { node }
    , info_ { info }
    , section_rule_ { section_rule }
    , stakeholder_ { stakeholder }
    , order_model_ { order_model }
    , product_model_ { product_model }
    , saved_ { !node->name.isEmpty() || node->party != 0 }
{
    ui->setupUi(this);

    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    IniCombo(ui->comboParty);
    IniCombo(ui->comboEmployee);
    IniDialog();
    IniConnect();

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

EditNodeOrder::~EditNodeOrder() { delete ui; }

void EditNodeOrder::accept()
{
    if (!saved_) {
        emit QDialog::accepted();
        saved_ = true;
        return;
    }

    order_model_->UpdateNode(node_);
}

void EditNodeOrder::reject()
{
    if (!saved_)
        ResourcePool<Node>::Instance().Recycle(node_);

    QDialog::reject();
    this->close();
}

void EditNodeOrder::RUpdatePartyEmployee()
{
    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_->model->ComboPathUnit(ui->comboEmployee, UNIT_EMPLOYEE);
    int unit { info_.section == Section::kSales ? 1 : 2 };
    stakeholder_->model->ComboPathUnit(ui->comboParty, unit);

    ui->comboEmployee->model()->sort(0);
    ui->comboParty->model()->sort(0);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);

    auto index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    auto index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);
}

void EditNodeOrder::on_chkBoxBranch_toggled(bool checked)
{
    ui->comboParty->setCurrentIndex(-1);
    ui->comboEmployee->setCurrentIndex(-1);

    SetWidgetsEnabledBranch(!checked);

    ui->labelParty->setText(checked ? tr("Branch") : tr("Company"));
    ui->comboParty->setInsertPolicy(checked ? QComboBox::InsertAtBottom : QComboBox::NoInsert);
    node_->branch = checked;
    EnabledPost(false);
}

void EditNodeOrder::IniDialog()
{
    ui->comboParty->setFocus();
    ui->comboParty->lineEdit()->setValidator(new QRegularExpressionValidator(QRegularExpression("[\\p{L} ()（）\\d]*"), this));

    // stakeholder-U: Employee = 0, Customer = 1, Vendor = 2, Product = 3
    int unit { info_.section == Section::kSales ? UNIT_CUSTOMER : UNIT_VENDOR };

    IniCombo(ui->comboParty, unit);
    IniCombo(ui->comboEmployee, UNIT_EMPLOYEE);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setDisplayFormat(DATE_FST);

    EnabledPost(false);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinDiscount->setDecimals(section_rule_.value_decimal);
    ui->dSpinFinalTotal->setRange(DMIN, DMAX);
    ui->dSpinFinalTotal->setDecimals(section_rule_.value_decimal);
    ui->dSpinInitialTotal->setRange(DMIN, DMAX);
    ui->dSpinInitialTotal->setDecimals(section_rule_.value_decimal);
}

void EditNodeOrder::IniCombo(QComboBox* combo)
{
    if (!combo)
        return;

    combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combo->setFrame(false);
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);

    auto completer { new QCompleter(combo->model(), combo) };
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    combo->setCompleter(completer);
}

void EditNodeOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_->model->ComboPathUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void EditNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &EditNodeOrder::accept); }

void EditNodeOrder::SetData() { }

void EditNodeOrder::SetWidgetsEnabled(bool enabled)
{
    ui->labelEmployee->setEnabled(enabled);
    ui->labelDiscount->setEnabled(enabled);
    ui->pBtnInsertParty->setEnabled(enabled);
    ui->comboEmployee->setEnabled(enabled);
    ui->dateEdit->setEnabled(enabled);
    ui->chkBoxRefund->setEnabled(enabled);
    ui->dSpinDiscount->setEnabled(enabled);
    ui->labelInitialTotal->setEnabled(enabled);
    ui->dSpinInitialTotal->setEnabled(enabled);
}

void EditNodeOrder::SetWidgetsEnabledBranch(bool enabled)
{
    SetWidgetsEnabled(enabled);

    ui->pBtnPrint->setEnabled(enabled);
    ui->dSpinFinalTotal->setEnabled(enabled);
    ui->pBtnInsertOrder->setEnabled(enabled);
}

void EditNodeOrder::SetWidgetsEnabledPost(bool enabled)
{
    SetWidgetsEnabled(enabled);

    ui->labelParty->setEnabled(enabled);
    ui->comboParty->setEnabled(enabled);
    ui->rBtnCash->setEnabled(enabled);
    ui->rBtnMonthly->setEnabled(enabled);
    ui->chkBoxBranch->setEnabled(enabled);
    ui->pBtnSaveOrder->setEnabled(enabled);
    ui->pBtnDeleteOrder->setEnabled(enabled);
    ui->rBtnPending->setEnabled(enabled);
}

void EditNodeOrder::ZeroSettlement()
{
    ui->dSpinDiscount->setValue(0.0);
    ui->dSpinFinalTotal->setValue(0.0);
}

void EditNodeOrder::EnabledPost(bool enabled) { ui->pBtnPostOrder->setEnabled(enabled); }

void EditNodeOrder::on_comboParty_currentTextChanged(const QString& arg1)
{
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;
    EnabledPost(true);
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    if (node_->branch)
        return;

    auto party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    EnabledPost(true);
    node_->party = party_id;

    auto stakehoder_model { stakeholder_->model };

    auto employee_index { ui->comboEmployee->findData(stakehoder_model->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakehoder_model->NodeRule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakehoder_model->NodeRule(party_id) == 1);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->posted = checked; }

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    node_->employee = ui->comboEmployee->currentData().toInt();
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ZeroSettlement();
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ZeroSettlement();
}

void EditNodeOrder::on_pBtnInsertParty_clicked()
{
    auto name { ui->comboParty->currentText() };
    if (node_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto model { stakeholder_->model };

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->node_rule = model->NodeRule(-1);
    model->SetParent(node, -1);
    node->name = name;

    int unit { info_.section == Section::kSales ? 1 : 2 };
    node->unit = unit;

    if (model->InsertNode(0, QModelIndex(), node)) {
        auto index = model->index(0, 0, QModelIndex());
        stakeholder_->widget->SetCurrentIndex(index);
    }
}

void EditNodeOrder::on_dateEdit_dateChanged(const QDate& date) { node_->date_time = date.toString(DATE_FST); }

void EditNodeOrder::on_pBtnPostOrder_toggled(bool checked)
{
    node_->posted = checked;
    SetWidgetsEnabledPost(!checked);
    ui->pBtnPostOrder->setText(checked ? tr("UnPost") : tr("Post"));

    if (checked) {
        accept();
        ui->pBtnPrint->setFocus();
    }

    ui->pBtnPrint->setDefault(checked);
}

void EditNodeOrder::on_dSpinDiscount_editingFinished()
{
    auto discount { ui->dSpinDiscount };

    if (discount->cleanText().isEmpty())
        discount->setValue(0.0);

    node_->initial_total -= (discount->value() - node_->discount);
    node_->discount = discount->value();
    ui->dSpinInitialTotal->setValue(node_->initial_total);
}
