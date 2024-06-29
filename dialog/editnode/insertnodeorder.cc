#include "insertnodeorder.h"

#include <QCompleter>

#include "component/constvalue.h"
#include "component/data.h"
#include "global/nodepool.h"
#include "ui_insertnodeorder.h"

InsertNodeOrder::InsertNodeOrder(Node* node, const SectionRule* section_rule, Tree* stakeholder, CStringHash* product_leaf, const Info* info, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeOrder)
    , node_ { node }
    , stakeholder_node_hash_ { stakeholder->model->GetNodeHash() }
    , stakeholder_branch_ { stakeholder->model->BranchPath() }
    , product_leaf_ { product_leaf }
    , info_ { info }
    , section_rule_ { section_rule }
    , stakeholder_ { stakeholder }
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

    int mark { info_->section == Section::kSales ? 1 : 2 };

    IniCombo(ui->comboCompany, stakeholder_node_hash_, stakeholder_branch_, mark);
    IniCombo(ui->comboEmployee, stakeholder_node_hash_, stakeholder_branch_, 0);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setDisplayFormat(DATE_FST);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinDiscount->setDecimals(section_rule_->value_decimal);
    ui->dSpinFinalTotal->setRange(DMIN, DMAX);
    ui->dSpinFinalTotal->setDecimals(section_rule_->value_decimal);
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

void InsertNodeOrder::IniCombo(QComboBox* combo, const NodeHash* node_hash, CStringHash* path_hash, int mark)
{
    for (auto it = path_hash->cbegin(); it != path_hash->cend(); ++it)
        if (node_hash->value(it.key())->unit == mark)
            combo->addItem(it.value(), it.key());

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
    auto stakeholder_node { stakeholder_node_hash_->value(stakeholder_id) };
    node_->seventh_property = stakeholder_id;

    auto employee_index { ui->comboEmployee->findData(stakeholder_node->second_property) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_node->node_rule == 0);
    ui->rBtnMonthly->setChecked(stakeholder_node->node_rule == 1);
}

void InsertNodeOrder::on_chkBoxRefund_toggled(bool checked) { node_->fifth_property = checked; }

void InsertNodeOrder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    if (node_->branch)
        return;

    node_->second_property = ui->comboEmployee->currentData().toInt();
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

    auto parent_index { QModelIndex() };
    Node* parent_node { model->GetNode(parent_index) };

    auto node { NodePool::Instance().Allocate() };
    node->node_rule = parent_node->node_rule;
    node->parent = parent_node;
    node->branch = true;
    node->name = name;

    int mark { info_->section == Section::kSales ? 1 : 2 };
    node->unit = mark;

    if (model->InsertRow(0, QModelIndex(), node)) {
        auto index = model->index(0, 0, QModelIndex());
        stakeholder_->widget->SetCurrentIndex(index);
    }

    ui->comboCompany->addItem(name, node->id);
}

void InsertNodeOrder::on_dateEdit_dateChanged(const QDate& date) { node_->date_time = date.toString(DATE_FST); }
