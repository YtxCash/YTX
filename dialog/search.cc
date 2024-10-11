#include "search.h"

#include <QHeaderView>

#include "component/enumclass.h"
#include "delegate/checkbox.h"
#include "delegate/checkboxr.h"
#include "delegate/search/searchcombor.h"
#include "delegate/table/colorr.h"
#include "delegate/table/tabledbclick.h"
#include "delegate/table/tabledoublespinr.h"
#include "delegate/tree/treecombor.h"
#include "delegate/tree/treedoublespinr.h"
#include "dialog/signalblocker.h"
#include "ui_search.h"

Search::Search(
    CInfo& info, const TreeModel* tree, const TreeModel* stakeholder_tree, SPSqlite sql, CStringHash& rule_hash, CSettings& settings, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Search)
    , sql_ { sql }
    , tree_ { tree }
    , stakeholder_tree_ { stakeholder_tree }
    , settings_ { settings }
    , info_ { info }
    , rule_hash_ { rule_hash }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    search_tree_ = new SearchNodeModel(info_, tree_, sql, this);
    search_table_ = new SearchTransModel(&info, sql, this);

    TreeViewDelegate(ui->treeView, search_tree_);
    TableViewDelegate(ui->tableView, search_table_);
    IniConnect();

    IniView(ui->treeView);
    IniView(ui->tableView);

    ResizeTreeColumn(ui->treeView->horizontalHeader());
    ResizeTableColumn(ui->tableView->horizontalHeader());

    IniDialog();
    HideTreeColumn(ui->treeView, info.section);
    HideTableColumn(ui->tableView, info.section);
}

Search::~Search() { delete ui; }

void Search::IniDialog()
{
    ui->rBtnNode->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);

    ui->pBtnCancel->setAutoDefault(false);
    ui->page->setContentsMargins(0, 0, 0, 0);
    ui->page2->setContentsMargins(0, 0, 0, 0);
    this->setWindowTitle(tr("Search"));
}

void Search::IniConnect()
{
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &Search::close);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &Search::RSearch);
    connect(ui->treeView, &QTableView::doubleClicked, this, &Search::RDoubleClicked);
    connect(ui->tableView, &QTableView::doubleClicked, this, &Search::RDoubleClicked);
}

void Search::HideTreeColumn(QTableView* view, Section section)
{
    switch (section) {
    case Section::kFinance:
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kEmployee), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kFirst), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kSecond), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kLocked), true);
        break;
    case Section::kTask:
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kEmployee), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kLocked), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kSecond), true);
        break;
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kEmployee), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kLocked), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kAmount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kSettled), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumOrder::kLocked), true);
        break;
    default:
        break;
    }
}

void Search::HideTableColumn(QTableView* view, Section section)
{
    switch (section) {
    case Section::kFinance:
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kUnitPrice), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kNodeID), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDiscountPrice), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSettled), true);
        break;
    case Section::kTask:
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kNodeID), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDiscountPrice), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kRhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSettled), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsDebit), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsCredit), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDiscountPrice), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kNodeID), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kRhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kRhsDebit), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kRhsCredit), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSettled), true);
        break;
    case Section::kPurchase:
    case Section::kSales:
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kRhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDocument), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kState), true);
    default:
        break;
    }
}

void Search::TreeViewDelegate(QTableView* view, SearchNodeModel* model)
{
    view->setModel(model);

    auto unit { new TreeComboR(info_.unit_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kUnit), unit);

    auto rule { new TreeComboR(rule_hash_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kRule), rule);

    auto total { new TreeDoubleSpinR(settings_.value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kAmount), total);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSettled), total);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDiscount), total);

    auto check { new CheckBoxR(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kBranch), check);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kLocked), check);

    auto name { new SearchComboR(tree_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kName), name);

    if (info_.section == Section::kProduct || info_.section == Section::kTask) {
        auto color { new ColorR(view) };
        view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDateTime), color);
    }

    auto party { new SearchComboR(stakeholder_tree_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kParty), party);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kEmployee), party);

    auto value { new TableDoubleSpinR(settings_.value_decimal, false, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFirst), value);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSecond), value);

    // view->setColumnHidden(std::to_underlying(TreeEnum::kID), true);
}

void Search::TableViewDelegate(QTableView* view, SearchTransModel* model)
{
    view->setModel(model);

    auto value { new TableDoubleSpinR(settings_.value_decimal, false, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kSettled), value);

    auto ratio { new TableDoubleSpinR(settings_.ratio_decimal, false, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsRatio), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsRatio), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kDiscountPrice), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kUnitPrice), ratio);

    if (info_.section == Section::kFinance || info_.section == Section::kTask || info_.section == Section::kProduct) {
        auto node_name { new SearchComboR(tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), node_name);
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), node_name);
    }

    if (info_.section == Section::kStakeholder) {
        auto lhs_node_name { new SearchComboR(tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsNode), lhs_node_name);

        auto rhs_node_name { new SearchComboR(stakeholder_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), rhs_node_name);
    }

    if (info_.section == Section::kSales || info_.section == Section::kPurchase) {
        auto node_name { new SearchComboR(stakeholder_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsNode), node_name);
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), node_name);
    }

    auto state { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kState), state);

    auto document { new TableDbClick(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kDocument), document);

    // view->setColumnHidden(std::to_underlying(TableEnumSearch::kID), true);
}

void Search::IniView(QTableView* view)
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->verticalHeader()->setHidden(true);
}

void Search::ResizeTreeColumn(QHeaderView* header)
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TreeEnumOrder::kDescription), QHeaderView::Stretch);
}

void Search::ResizeTableColumn(QHeaderView* header)
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TableEnumSearch::kDescription), QHeaderView::Stretch);
}

void Search::RSearch()
{
    CString kText { ui->lineEdit->text() };

    if (ui->rBtnNode->isChecked()) {
        search_tree_->Query(kText);
        ResizeTreeColumn(ui->treeView->horizontalHeader());
    }

    if (ui->rBtnTransaction->isChecked()) {
        search_table_->Query(kText);
        ResizeTableColumn(ui->tableView->horizontalHeader());
    }
}

void Search::RDoubleClicked(const QModelIndex& index)
{
    if (ui->rBtnNode->isChecked()) {
        int node_id { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() };
        emit STreeLocation(node_id);
    }

    if (ui->rBtnTransaction->isChecked()) {
        int lhs_node_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kLhsNode)).data().toInt() };
        int rhs_node_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kRhsNode)).data().toInt() };
        int trans_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kID)).data().toInt() };
        emit STableLocation(trans_id, lhs_node_id, rhs_node_id);
    }
}

void Search::on_rBtnNode_toggled(bool checked)
{
    if (checked)
        ui->stackedWidget->setCurrentIndex(0);
}

void Search::on_rBtnTransaction_toggled(bool checked)
{
    if (checked)
        ui->stackedWidget->setCurrentIndex(1);
}
