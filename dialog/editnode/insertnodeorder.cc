#include "insertnodeorder.h"

#include <QCompleter>

#include "component/constvalue.h"
#include "component/data.h"
#include "global/resourcepool.h"
#include "ui_insertnodeorder.h"

InsertNodeOrder::InsertNodeOrder(
    Node* node, CSectionRule& section_rule, Tree* stakeholder, const TreeModel& product_model, CInfo& info, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeOrder)
    , node_ { node }
    , info_ { info }
    , section_rule_ { section_rule }
    , stakeholder_ { stakeholder }
    , product_model_ { product_model }
{
    ui->setupUi(this);

    ui->comboCompany->blockSignals(true);
    ui->comboEmployee->blockSignals(true);

    IniCombo(ui->comboCompany);
    IniCombo(ui->comboEmployee);
    IniDialog();
    IniConnect();

    ui->comboCompany->blockSignals(false);
    ui->comboEmployee->blockSignals(false);
}

InsertNodeOrder::~InsertNodeOrder() { delete ui; }

void InsertNodeOrder::on_chkBoxBranch_toggled(bool checked)
{
    ui->comboCompany->setCurrentIndex(-1);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->label_5->setEnabled(!checked);
    ui->label_6->setEnabled(!checked);
    ui->pBtnPrint->setEnabled(!checked);
    ui->pBtnInsertStakeholder->setEnabled(!checked);
    ui->dSpinFinalTotal->setEnabled(!checked);
    ui->comboEmployee->setEnabled(!checked);
    ui->dateEdit->setEnabled(!checked);
    ui->chkBoxRefund->setEnabled(!checked);
    ui->dSpinDiscount->setEnabled(!checked);

    ui->label->setText(checked ? tr("Branch") : tr("Company"));
    ui->comboCompany->setInsertPolicy(checked ? QComboBox::InsertAtBottom : QComboBox::NoInsert);
    node_->branch = checked;
}

void InsertNodeOrder::IniDialog()
{
    ui->comboCompany->setFocus();
    ui->comboCompany->lineEdit()->setValidator(new QRegularExpressionValidator(QRegularExpression("[\\p{L} ()（）\\d]*"), this));

    int mark { info_.section == Section::kSales ? 1 : 2 };

    IniCombo(ui->comboCompany, mark);
    IniCombo(ui->comboEmployee, 0);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setDisplayFormat(DATE_FST);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinDiscount->setDecimals(section_rule_.value_decimal);
    ui->dSpinFinalTotal->setRange(DMIN, DMAX);
    ui->dSpinFinalTotal->setDecimals(section_rule_.value_decimal);
}

void InsertNodeOrder::IniCombo(QComboBox* combo)
{
    combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combo->setFrame(false);
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);

    auto completer { new QCompleter(combo->model(), combo) };
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    combo->setCompleter(completer);
}

void InsertNodeOrder::IniCombo(QComboBox* combo, int mark)
{
    stakeholder_->model->ComboPathUnit(combo, mark);
    combo->model()->sort(0);
    combo->setCurrentIndex(-1);
}

void InsertNodeOrder::IniConnect() { }

void InsertNodeOrder::SetData() { }

void InsertNodeOrder::ZeroSettlement()
{
    ui->dSpinDiscount->setValue(0.0);
    ui->dSpinFinalTotal->setValue(0.0);
}

void InsertNodeOrder::on_comboCompany_currentTextChanged(const QString& arg1)
{
    if (!node_->branch || arg1.isEmpty())
        return;

    node_->name = arg1;
}

void InsertNodeOrder::on_comboCompany_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    if (node_->branch)
        return;

    auto stakeholder_id { ui->comboCompany->currentData().toInt() };
    node_->party = stakeholder_id;

    auto stakehoder_model { stakeholder_->model };

    auto employee_index { ui->comboEmployee->findData(stakehoder_model->Employee(stakeholder_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakehoder_model->NodeRule(stakeholder_id) == 0);
    ui->rBtnMonthly->setChecked(stakehoder_model->NodeRule(stakeholder_id) == 1);
}

void InsertNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->fourth = checked; }

void InsertNodeOrder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    if (node_->branch)
        return;

    node_->employee = ui->comboEmployee->currentData().toInt();
}

void InsertNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = 0;
}

void InsertNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = 1;
    ZeroSettlement();
}

void InsertNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = 2;
    ZeroSettlement();
}

void InsertNodeOrder::on_pBtnInsertStakeholder_clicked()
{
    auto name { ui->comboCompany->currentText() };
    if (node_->branch || name.isEmpty() || ui->comboCompany->currentIndex() != -1)
        return;

    auto model { stakeholder_->model };

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->node_rule = model->NodeRule(-1);
    model->SetParent(node, -1);
    node->name = name;

    int mark { info_.section == Section::kSales ? 1 : 2 };
    node->unit = mark;

    if (model->InsertNode(0, QModelIndex(), node)) {
        auto index = model->index(0, 0, QModelIndex());
        stakeholder_->widget->SetCurrentIndex(index);
    }

    ui->comboCompany->addItem(name, node->id);
}

void InsertNodeOrder::on_dateEdit_dateChanged(const QDate& date) { node_->date_time = date.toString(DATE_FST); }
