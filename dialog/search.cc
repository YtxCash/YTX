#include "search.h"

#include <QHeaderView>

#include "component/enumclass.h"
#include "delegate/checkbox.h"
#include "delegate/checkboxr.h"
#include "delegate/search/searchpathtabler.h"
#include "delegate/search/searchpathtreer.h"
#include "delegate/table/colorr.h"
#include "delegate/table/tabledbclick.h"
#include "delegate/table/tabledoublespinr.h"
#include "delegate/tree/treecombor.h"
#include "delegate/tree/treedoublespinr.h"
#include "dialog/signalblocker.h"
#include "ui_search.h"

Search::Search(CInfo& info, CTreeModel* tree, CTreeModel* stakeholder_tree, CTreeModel* product_tree, Sqlite* sql, CStringMap& rule_map, CSettings& settings,
    QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Search)
    , sql_ { sql }
    , tree_ { tree }
    , stakeholder_tree_ { stakeholder_tree }
    , product_tree_ { product_tree }
    , settings_ { settings }
    , info_ { info }
    , rule_map_ { rule_map }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    search_tree_ = new SearchNodeModel(info_, tree_, stakeholder_tree, sql, this);
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
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kEmployee), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kColor), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kFirst), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kSecond), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kFinished), true);
        break;
    case Section::kTask:
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kEmployee), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kSecond), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kFinished), true);
        break;
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kEmployee), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kFinished), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kParty), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kColor), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kFinished), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kInitialTotal), true);
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kFinalTotal), true);
        break;
    case Section::kSales:
    case Section::kPurchase:
        view->setColumnHidden(std::to_underlying(TreeEnumSearch::kColor), true);
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
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSupportID), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDiscountPrice), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSettled), true);
        break;
    case Section::kTask:
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSupportID), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDiscountPrice), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kRhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kSettled), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsRatio), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsDebit), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kLhsCredit), true);
        view->setColumnHidden(std::to_underlying(TableEnumSearch::kDiscountPrice), true);
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

    auto* unit { new TreeComboR(info_.unit_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kUnit), unit);

    auto* rule { new TreeComboR(rule_map_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kRule), rule);

    auto* total { new TreeDoubleSpinR(settings_.amount_decimal, true, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kInitialTotal), total);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kFinalTotal), total);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kDiscount), total);

    auto* check { new CheckBoxR(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kType), check);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kFinished), check);

    auto* name { new SearchPathTreeR(tree_, std::to_underlying(TreeEnumSearch::kID), view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kName), name);

    if (info_.section == Section::kProduct || info_.section == Section::kTask) {
        auto* color { new ColorR(view) };
        view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kColor), color);
    }

    auto* stakeholder { new SearchPathTableR(stakeholder_tree_, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kParty), stakeholder);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kEmployee), stakeholder);

    auto* value { new TableDoubleSpinR(settings_.amount_decimal, false, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kFirst), value);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumSearch::kSecond), value);

    // view->setColumnHidden(std::to_underlying(TreeEnum::kID), true);
}

void Search::TableViewDelegate(QTableView* view, SearchTransModel* model)
{
    view->setModel(model);

    auto* value { new TableDoubleSpinR(settings_.amount_decimal, false, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kSettled), value);

    auto* ratio { new TableDoubleSpinR(settings_.common_decimal, false, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsRatio), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsRatio), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kDiscountPrice), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kUnitPrice), ratio);

    if (info_.section == Section::kFinance || info_.section == Section::kTask || info_.section == Section::kProduct) {
        auto* node_name { new SearchPathTableR(tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsNode), node_name);
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), node_name);
    }

    if (info_.section == Section::kStakeholder) {
        auto* rhs_node_name { new SearchPathTableR(tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), rhs_node_name);

        auto* lhs_node_name { new SearchPathTableR(product_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kLhsNode), lhs_node_name);
    }

    if (info_.section == Section::kSales || info_.section == Section::kPurchase) {
        auto* rhs_node_name { new SearchPathTableR(stakeholder_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), rhs_node_name);

        auto* lhs_node_name { new SearchPathTableR(product_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kRhsNode), lhs_node_name);
    }

    auto* state { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumSearch::kState), state);

    auto* document { new TableDbClick(view) };
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
    header->setSectionResizeMode(std::to_underlying(TreeEnumSearch::kDescription), QHeaderView::Stretch);
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
        int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
        emit STreeLocation(node_id);
    }

    if (ui->rBtnTransaction->isChecked()) {
        int lhs_node_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kLhsNode)).data().toInt() };
        int rhs_node_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kRhsNode)).data().toInt() };
        int trans_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kID)).data().toInt() };
        int node_id { index.siblingAtColumn(std::to_underlying(TableEnumSearch::kSupportID)).data().toInt() };

        switch (info_.section) {
        case Section::kStakeholder:
            emit STableLocation(trans_id, node_id, 0);
            break;
        case Section::kFinance:
        case Section::kProduct:
        case Section::kTask:
            emit STableLocation(trans_id, lhs_node_id, rhs_node_id);
            break;
        default:
            break;
        }
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
