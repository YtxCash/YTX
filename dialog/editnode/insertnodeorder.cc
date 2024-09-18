#include "insertnodeorder.h"

#include <QCompleter>
#include <QMessageBox>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "ui_insertnodeorder.h"

InsertNodeOrder::InsertNodeOrder(Node* node, CSectionRule& section_rule, AbstractTreeModel* order_model, AbstractTreeModel* stakeholder_model,
    const AbstractTreeModel& product_model, CInfo& info, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeOrder)
    , node_ { node }
    , info_ { info }
    , section_rule_ { section_rule }
    , stakeholder_model_ { stakeholder_model }
    , order_model_ { order_model }
    , product_model_ { product_model }
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

InsertNodeOrder::~InsertNodeOrder() { delete ui; }

void InsertNodeOrder::RUpdateStakeholder()
{
    ui->comboParty->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_model_->ComboPathUnit(ui->comboEmployee, UNIT_EMPLOYEE);
    int unit { info_.section == Section::kSales ? 1 : 2 };
    stakeholder_model_->ComboPathUnit(ui->comboParty, unit);

    ui->comboEmployee->model()->sort(0);
    ui->comboParty->model()->sort(0);

    ui->comboParty->blockSignals(false);
    ui->comboEmployee->blockSignals(false);

    auto index_employee { ui->comboEmployee->findData(employee_id) };
    ui->comboEmployee->setCurrentIndex(index_employee);

    auto index_party { ui->comboParty->findData(party_id) };
    ui->comboParty->setCurrentIndex(index_party);
}

void InsertNodeOrder::on_chkBoxBranch_toggled(bool checked)
{
    ui->comboEmployee->setCurrentIndex(-1);

    SetWidgetsEnabledBranch(!checked);

    ui->labelParty->setText(checked ? tr("Branch") : tr("Company"));
    ui->comboParty->setInsertPolicy(checked ? QComboBox::InsertAtBottom : QComboBox::NoInsert);
    node_->branch = checked;
    EnabledPost(false);
}

void InsertNodeOrder::IniDialog()
{
    ui->comboParty->setFocus();
    ui->comboParty->lineEdit()->setValidator(&LineEdit::GetInputValidator());

    // stakeholder-U: Employee = 0, Customer = 1, Vendor = 2, Product = 3
    int unit { info_.section == Section::kSales ? UNIT_CUSTOMER : UNIT_VENDOR };

    IniCombo(ui->comboParty, unit);
    IniCombo(ui->comboEmployee, UNIT_EMPLOYEE);

    ui->dateTimeEdit->setDate(QDate::currentDate());
    ui->dateTimeEdit->setDisplayFormat(DATE_FST);

    EnabledPost(false);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinDiscount->setDecimals(section_rule_.value_decimal);
    ui->dSpinFinalTotal->setRange(DMIN, DMAX);
    ui->dSpinFinalTotal->setDecimals(section_rule_.value_decimal);
    ui->dSpinInitialTotal->setRange(DMIN, DMAX);
    ui->dSpinInitialTotal->setDecimals(section_rule_.value_decimal);
}

void InsertNodeOrder::IniCombo(QComboBox* combo)
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

void InsertNodeOrder::IniCombo(QComboBox* combo, int unit)
{
    if (!combo)
        return;

    stakeholder_model_->ComboPathUnit(combo, unit);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void InsertNodeOrder::accept()
{
    if (is_modified_) {
        if (!is_saved_) {
            emit QDialog::accepted();
            is_saved_ = true;
        }

        if (is_saved_)
            order_model_->UpdateNode(node_);

        is_modified_ = false;
    }
}

void InsertNodeOrder::reject()
{
    if (!is_saved_)
        ResourcePool<Node>::Instance().Recycle(node_);

    QDialog::reject();
}

void InsertNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &InsertNodeOrder::accept); }

void InsertNodeOrder::SetData() { }

void InsertNodeOrder::SetWidgetsEnabled(bool enabled)
{
    ui->labelEmployee->setEnabled(enabled);
    ui->labelDiscount->setEnabled(enabled);
    ui->pBtnInsertParty->setEnabled(enabled);
    ui->comboEmployee->setEnabled(enabled);
    ui->dateTimeEdit->setEnabled(enabled);
    ui->chkBoxRefund->setEnabled(enabled);
    ui->dSpinDiscount->setEnabled(enabled);
    ui->labelInitialTotal->setEnabled(enabled);
    ui->dSpinInitialTotal->setEnabled(enabled);
}

void InsertNodeOrder::SetWidgetsEnabledBranch(bool enabled)
{
    SetWidgetsEnabled(enabled);

    ui->pBtnPrint->setEnabled(enabled);
    ui->dSpinFinalTotal->setEnabled(enabled);
}

void InsertNodeOrder::SetWidgetsEnabledPost(bool enabled)
{
    SetWidgetsEnabled(enabled);

    ui->labelParty->setEnabled(enabled);
    ui->comboParty->setEnabled(enabled);
    ui->rBtnCash->setEnabled(enabled);
    ui->rBtnMonthly->setEnabled(enabled);
    ui->chkBoxBranch->setEnabled(enabled);
    ui->pBtnSaveOrder->setEnabled(enabled);
    ui->rBtnPending->setEnabled(enabled);
}

void InsertNodeOrder::ZeroSettlement()
{
    ui->dSpinDiscount->setValue(0.0);
    ui->dSpinFinalTotal->setValue(0.0);
}

void InsertNodeOrder::EnabledPost(bool enabled) { ui->pBtnPostOrder->setEnabled(enabled); }

void InsertNodeOrder::on_comboParty_currentTextChanged(const QString& arg1)
{
    if (arg1.isEmpty())
        return;

    if (!node_->branch) {
        node_->name = arg1;
        is_modified_ = true;
        return;
    }

    EnabledPost(true);
}

void InsertNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (node_->branch)
        return;

    auto party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    EnabledPost(true);
    is_modified_ = true;

    node_->party = party_id;

    auto employee_index { ui->comboEmployee->findData(stakeholder_model_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_model_->NodeRule(party_id) == 0);
    ui->rBtnMonthly->setChecked(stakeholder_model_->NodeRule(party_id) == 1);
}

void InsertNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->node_rule = checked; }

void InsertNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/) { node_->employee = ui->comboEmployee->currentData().toInt(); }

void InsertNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_CASH;
}

void InsertNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_MONTHLY;
    ZeroSettlement();
}

void InsertNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = UNIT_PENDING;
    ZeroSettlement();
}

void InsertNodeOrder::on_pBtnInsertParty_clicked()
{
    auto name { ui->comboParty->currentText() };
    if (node_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->node_rule = stakeholder_model_->NodeRule(-1);
    stakeholder_model_->SetParent(node, -1);
    node->name = name;

    int unit { info_.section == Section::kSales ? 1 : 2 };
    node->unit = unit;

    stakeholder_model_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void InsertNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time) { node_->date_time = date_time.toString(DATE_TIME_FST); }

void InsertNodeOrder::on_pBtnPostOrder_toggled(bool checked)
{
    node_->locked = checked;
    SetWidgetsEnabledPost(!checked);
    ui->pBtnPostOrder->setText(checked ? tr("UnPost") : tr("Post"));

    if (checked) {
        accept();
        ui->pBtnPrint->setFocus();
    }

    ui->pBtnPrint->setDefault(checked);
}

void InsertNodeOrder::on_dSpinDiscount_editingFinished()
{
    auto discount { ui->dSpinDiscount };

    if (discount->cleanText().isEmpty())
        discount->setValue(0.0);

    node_->initial_total -= (discount->value() - node_->discount);
    node_->discount = discount->value();
    ui->dSpinInitialTotal->setValue(node_->initial_total);
}
