#include "search.h"

#include <QHeaderView>

#include "component/enumclass.h"
#include "delegate/checkbox.h"
#include "delegate/search/searchcombor.h"
#include "delegate/table/tablecombo.h"
#include "delegate/table/tabledbclick.h"
#include "delegate/table/tabledoublespinr.h"
#include "delegate/tree/treecombo.h"
#include "delegate/tree/treedoublespindynamicunitr.h"
#include "dialog/signalblocker.h"
#include "ui_search.h"

Search::Search(CInfo& info, CInterface& interface, const TreeModel& tree_model, QSharedPointer<SearchSqlite> sql, CSettings& settings, CStringHash& rule_hash,
    QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Search)
    , sql_ { sql }
    , rule_hash_ { rule_hash }
    , settings_ { settings }
    , tree_model_ { tree_model }
    , info_ { info }
    , interface_ { interface }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog();

    search_tree_model_ = new SearchTreeModel(info_, tree_model_, settings_, sql, this);
    search_table_model_ = new SearchTableModel(&info, sql, this);

    IniTree(ui->treeView, search_tree_model_);
    IniTable(ui->tableView, search_table_model_);
    IniConnect();

    IniView(ui->treeView);
    HideColumn(ui->treeView, info.section);
    IniView(ui->tableView);

    ResizeTreeColumn(ui->treeView->horizontalHeader());
    ResizeTableColumn(ui->tableView->horizontalHeader());
}

Search::~Search() { delete ui; }

void Search::IniDialog()
{
    ui->rBtnNode->setChecked(true);
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

void Search::IniTree(QTableView* view, SearchTreeModel* model)
{
    view->setModel(model);

    auto unit { new TreeCombo(info_.unit_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kUnit), unit);

    auto rule { new TreeCombo(rule_hash_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kRule), rule);

    auto total { new TreeDoubleSpinDynamicUnitR(settings_.value_decimal, info_.unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kInitialTotal), total);

    auto branch { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kBranch), branch);

    auto name { new SearchComboR(&tree_model_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kName), name);

    view->setColumnHidden(std::to_underlying(TreeEnum::kID), true);
    view->setColumnHidden(std::to_underlying(TreeEnum::kPlaceholder), true);
}

void Search::IniTable(QTableView* view, SearchTableModel* model)
{
    view->setModel(model);

    auto value { new TableDoubleSpinR(settings_.value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsCredit), value);

    auto ratio { new TableDoubleSpinR(settings_.ratio_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsRatio), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsRatio), ratio);

    auto node_name { new TableCombo(&tree_model_, 0, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), node_name);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsNode), node_name);

    auto state { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kState), state);

    auto document { new TableDbClick(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kDocument), document);

    view->setColumnHidden(std::to_underlying(TableEnumSearch::kID), true);
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

void Search::HideColumn(QTableView* view, Section section)
{
    switch (section) {
    case Section::kTask:
    case Section::kFinance:
        view->setColumnHidden(std::to_underlying(TreeEnum::kThird), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFirst), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kSecond), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFourth), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFinalTotal), true);
        break;
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(TreeEnum::kFirst), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kSecond), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFourth), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFinalTotal), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(TreeEnum::kInitialTotal), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kRule), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFourth), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kFinalTotal), true);
        view->setColumnHidden(std::to_underlying(TreeEnum::kSecond), true);
        break;
    default:
        break;
    }
}

void Search::ResizeTreeColumn(QHeaderView* header)
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
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
        search_tree_model_->Query(kText);
        ResizeTreeColumn(ui->treeView->horizontalHeader());
    }

    if (ui->rBtnTransaction->isChecked()) {
        search_table_model_->Query(kText);
        ResizeTableColumn(ui->tableView->horizontalHeader());
    }
}

void Search::RDoubleClicked(const QModelIndex& index)
{
    if (ui->rBtnNode->isChecked()) {
        int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
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
