#include "mainwindow.h"

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QLockFile>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QQueue>
#include <QResource>
#include <QScrollBar>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "database/sqlite/sqlitefinance.h"
#include "database/sqlite/sqliteproduct.h"
#include "database/sqlite/sqlitepurchase.h"
#include "database/sqlite/sqlitesales.h"
#include "database/sqlite/sqlitestakeholder.h"
#include "database/sqlite/sqlitetask.h"
#include "delegate/checkbox.h"
#include "delegate/datetime.h"
#include "delegate/line.h"
#include "delegate/table/tablecombo.h"
#include "delegate/table/tabledbclick.h"
#include "delegate/table/tabledoublespin.h"
#include "delegate/table/tabledoublespinr.h"
#include "delegate/tree/finance/financeforeign.h"
#include "delegate/tree/order/orderdatetime.h"
#include "delegate/tree/order/orderstakeholder.h"
#include "delegate/tree/order/ordertotal.h"
#include "delegate/tree/treecombo.h"
#include "delegate/tree/treedoublespin.h"
#include "delegate/tree/treedoublespindynamicunitr.h"
#include "delegate/tree/treedoublespinpercent.h"
#include "delegate/tree/treedoublespinr.h"
#include "delegate/tree/treedoublespinunitr.h"
#include "delegate/tree/treeplaintext.h"
#include "delegate/tree/treespin.h"
#include "delegate/tree/treespinr.h"
#include "dialog/about.h"
#include "dialog/editdocument.h"
#include "dialog/editnode/editnodefinance.h"
#include "dialog/editnode/editnodeorder.h"
#include "dialog/editnode/editnodeproduct.h"
#include "dialog/editnode/editnodestakeholder.h"
#include "dialog/editnode/insertnodeorder.h"
#include "dialog/preferences.h"
#include "dialog/removenode.h"
#include "dialog/search.h"
#include "global/resourcepool.h"
#include "global/signalstation.h"
#include "global/sqlconnection.h"
#include "table/model/tablemodelfinance.h"
#include "table/model/tablemodelorder.h"
#include "table/model/tablemodelproduct.h"
#include "table/model/tablemodelstakeholder.h"
#include "table/model/tablemodeltask.h"
#include "tree/model/treemodelfinancetask.h"
#include "tree/model/treemodelorder.h"
#include "tree/model/treemodelproduct.h"
#include "tree/model/treemodelstakeholder.h"
#include "ui_mainwindow.h"
#include "widget/tablewidget/tablewidgetcommon.h"
#include "widget/tablewidget/tablewidgetorder.h"
#include "widget/treewidget/treewidgetcommon.h"
#include "widget/treewidget/treewidgetorder.h"
#include "widget/treewidget/treewidgetstakeholder.h"

MainWindow::MainWindow(CString& dir_path, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , dir_path_ { dir_path }
{
    ResourceFile();
    SharedInterface(dir_path);

    ui->setupUi(this);
    SetTabWidget();
    SetConnect();
    SetDateFormat();
    SetHeader();
    SetAction();

    qApp->setWindowIcon(QIcon(":/logo/logo/logo.png"));
    this->setAcceptDrops(true);

    RestoreState(ui->splitter, shared_interface_, WINDOW, SPLITTER_STATE);
    RestoreState(this, shared_interface_, WINDOW, MAINWINDOW_STATE);
    RestoreGeometry(this, shared_interface_, WINDOW, MAINWINDOW_GEOMETRY);

    Recent();

#ifdef Q_OS_WIN
    ui->actionRemove->setShortcut(Qt::Key_Delete);
#elif defined(Q_OS_MACOS)
    ui->actionRemove->setShortcut(Qt::Key_Backspace);
#endif
}

MainWindow::~MainWindow()
{
    SaveState(ui->splitter, shared_interface_, WINDOW, SPLITTER_STATE);
    SaveState(this, shared_interface_, WINDOW, MAINWINDOW_STATE);
    SaveGeometry(this, shared_interface_, WINDOW, MAINWINDOW_GEOMETRY);
    shared_interface_->setValue(START_SECTION, std::to_underlying(start_));

    if (finance_tree_.widget) {
        SaveTableWidget(finance_table_, finance_data_.info.node, VIEW);
        SaveState(finance_tree_.widget->Header(), exclusive_interface_, finance_data_.info.node, HEADER_STATE);

        finance_dialog_.clear();
    }

    if (product_tree_.widget) {
        SaveTableWidget(product_table_, product_data_.info.node, VIEW);
        SaveState(product_tree_.widget->Header(), exclusive_interface_, product_data_.info.node, HEADER_STATE);

        product_dialog_.clear();
    }

    if (stakeholder_tree_.widget) {
        SaveTableWidget(stakeholder_table_, stakeholder_data_.info.node, VIEW);
        SaveState(stakeholder_tree_.widget->Header(), exclusive_interface_, stakeholder_data_.info.node, HEADER_STATE);

        stakeholder_dialog_.clear();
    }

    if (task_tree_.widget) {
        SaveTableWidget(task_table_, task_data_.info.node, VIEW);
        SaveState(task_tree_.widget->Header(), exclusive_interface_, task_data_.info.node, HEADER_STATE);

        task_dialog_.clear();
    }

    if (sales_tree_.widget) {
        SaveTableWidget(sales_table_, sales_data_.info.node, VIEW);
        SaveState(sales_tree_.widget->Header(), exclusive_interface_, sales_data_.info.node, HEADER_STATE);

        sales_dialog_.clear();
    }

    if (purchase_tree_.widget) {
        SaveTableWidget(purchase_table_, purchase_data_.info.node, VIEW);
        SaveState(purchase_tree_.widget->Header(), exclusive_interface_, purchase_data_.info.node, HEADER_STATE);

        purchase_dialog_.clear();
    }

    delete ui;
}

void MainWindow::ROpenFile(CString& file_path)
{
    if (file_path.isEmpty() || file_path == SqlConnection::Instance().DatabaseName())
        return;

    if (SqlConnection::Instance().DatabaseEnable()) {
        QProcess::startDetached(qApp->applicationFilePath(), QStringList() << file_path);
        return;
    }

    if (SqlConnection::Instance().SetDatabaseName(file_path)) {
        QFileInfo file_info(file_path);
        auto complete_base_name { file_info.completeBaseName() };

        this->setWindowTitle(complete_base_name);
        ExclusiveInterface(dir_path_, complete_base_name);
        LockFile(file_info.absolutePath(), complete_base_name);

        sql_ = MainwindowSqlite(start_);
        SetFinanceData();
        SetTaskData();
        SetProductData();
        SetStakeholderData();
        SetSalesData();
        SetPurchaseData();

        CreateSection(finance_tree_, tr(Finance), &finance_data_, &finance_table_, &finance_rule_);
        CreateSection(stakeholder_tree_, tr(Stakeholder), &stakeholder_data_, &stakeholder_table_, &stakeholder_rule_);
        CreateSection(product_tree_, tr(Product), &product_data_, &product_table_, &product_rule_);
        CreateSection(task_tree_, tr(Task), &task_data_, &task_table_, &task_rule_);
        CreateSection(sales_tree_, tr(Sales), &sales_data_, &sales_table_, &sales_rule_);
        CreateSection(purchase_tree_, tr(Purchase), &purchase_data_, &purchase_table_, &purchase_rule_);

        switch (start_) {
        case Section::kFinance:
            on_rBtnFinance_toggled(true);
            break;
        case Section::kStakeholder:
            on_rBtnStakeholder_toggled(true);
            break;
        case Section::kProduct:
            on_rBtnProduct_toggled(true);
            break;
        case Section::kTask:
            on_rBtnTask_toggled(true);
            break;
        case Section::kSales:
            on_rBtnSales_toggled(true);
            break;
        case Section::kPurchase:
            on_rBtnPurchase_toggled(true);
            break;
        default:
            break;
        }

        QString path { QDir::toNativeSeparators(file_path) };

        if (!recent_list_.contains(path)) {
            auto menu { ui->menuRecent };
            auto action { new QAction(path, menu) };

            if (menu->isEmpty()) {
                menu->addAction(action);
                SetClearMenuAction();
            } else
                ui->menuRecent->insertAction(ui->actionSeparator, action);

            recent_list_.emplaceBack(path);
            UpdateRecent();
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        auto suffix { QFileInfo(event->mimeData()->urls().at(0).fileName()).suffix().toLower() };
        if (suffix == YTX)
            return event->acceptProposedAction();
    }

    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) { ROpenFile(event->mimeData()->urls().at(0).toLocalFile()); }

void MainWindow::RTreeViewDoubleClicked(const QModelIndex& index)
{
    if (index.column() != 0)
        return;

    bool branch { index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool() };
    if (branch)
        return;

    auto node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    if (node_id == -1)
        return;

    if (!section_table_->contains(node_id))
        CreateTable(section_data_, section_tree_->model, section_rule_, section_table_, node_id);

    SwitchTab(node_id);
}

void MainWindow::SwitchTab(int node_id, int trans_id)
{
    auto widget { section_table_->value(node_id, nullptr) };
    if (!widget)
        return;

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();

    if (trans_id == 0)
        return;

    auto view { widget->View() };
    auto index { GetTableModel(view)->GetIndex(trans_id) };
    view->setCurrentIndex(index);
    view->scrollTo(index.siblingAtColumn(std::to_underlying(TableEnum::kDateTime)), QAbstractItemView::PositionAtCenter);
}

bool MainWindow::LockFile(CString& absolute_path, CString& complete_base_name)
{
    auto lock_file_path { absolute_path + SLASH + complete_base_name + SFX_LOCK };

    static QLockFile lock_file { lock_file_path };
    if (!lock_file.tryLock(100))
        return false;

    return true;
}

void MainWindow::CreateTable(SectionData* data, TreeModel* tree_model, CSectionRule* section_rule, TableHash* table_hash, int node_id)
{
    auto name { tree_model->Name(node_id) };
    auto section { data->info.section };
    auto node_rule { tree_model->NodeRule(node_id) };

    auto tmp_node { ResourcePool<Node>::Instance().Allocate() };
    tree_model->CopyNode(tmp_node, node_id);

    TableWidget* widget {};

    switch (section) {
    case Section::kSales:
        widget = new TableWidgetOrder(tmp_node, tree_model, stakeholder_tree_.model, *product_tree_.model, section_rule->value_decimal, UNIT_CUSTOMER, this);
        break;
    case Section::kPurchase:
        widget = new TableWidgetOrder(tmp_node, tree_model, stakeholder_tree_.model, *product_tree_.model, section_rule->value_decimal, UNIT_VENDOR, this);
        break;
    default:
        widget = new TableWidgetCommon(this);
        break;
    }

    TableModel* model {};

    switch (section) {
    case Section::kFinance:
        model = new TableModelFinance(data->sql, node_rule, node_id, data->info, finance_rule_, this);
        break;
    case Section::kProduct:
        model = new TableModelProduct(data->sql, node_rule, node_id, data->info, product_rule_, this);
        break;
    case Section::kTask:
        model = new TableModelTask(data->sql, node_rule, node_id, data->info, task_rule_, this);
        break;
    case Section::kStakeholder:
        model = new TableModelStakeholder(data->sql, node_rule, node_id, data->info, stakeholder_rule_, this);
        break;
    case Section::kSales:
        model = new TableModelOrder(data->sql, node_rule, node_id, data->info, *section_rule, this);
        break;
    case Section::kPurchase:
        model = new TableModelOrder(data->sql, node_rule, node_id, data->info, *section_rule, this);
        break;
    default:
        break;
    }

    widget->SetModel(model);

    int tab_index { ui->tabWidget->addTab(widget, name) };
    auto tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    if (section != Section::kSales && section != Section::kPurchase) {
        auto view { widget->View() };
        SetView(view);
        SetConnect(view, model, tree_model, data);

        switch (section) {
        case Section::kFinance:
        case Section::kProduct:
        case Section::kTask:
            CreateDelegate(view, tree_model, section_rule, node_id);
            break;
        case Section::kStakeholder:
            DelegateStakeholder(view, section_rule);
            break;
        default:
            break;
        }
    }

    table_hash->insert(node_id, widget);
    SignalStation::Instance().RegisterModel(section, node_id, model);
}

void MainWindow::SetConnect(const QTableView* view, const TableModel* table, const TreeModel* tree, const SectionData* data)
{
    connect(table, &TableModel::SResizeColumnToContents, view, &QTableView::resizeColumnToContents);

    connect(table, &TableModel::SRemoveOne, &SignalStation::Instance(), &SignalStation::RRemoveOne);
    connect(table, &TableModel::SAppendOne, &SignalStation::Instance(), &SignalStation::RAppendOne);
    connect(table, &TableModel::SUpdateBalance, &SignalStation::Instance(), &SignalStation::RUpdateBalance);
    connect(table, &TableModel::SMoveMultiTrans, &SignalStation::Instance(), &SignalStation::RMoveMultiTrans);

    connect(table, &TableModel::SUpdateOneTotal, tree, &TreeModel::RUpdateOneTotal);
    connect(table, &TableModel::SSearch, tree, &TreeModel::RSearch);

    connect(tree, &TreeModel::SNodeRule, table, &TableModel::RNodeRule);

    connect(data->sql.data(), &Sqlite::SRemoveMultiTrans, table, &TableModel::RRemoveMultiTrans);

    connect(data->sql.data(), &Sqlite::SMoveMultiTrans, &SignalStation::Instance(), &SignalStation::RMoveMultiTrans);
}

void MainWindow::CreateDelegate(QTableView* view, const TreeModel* tree_model, CSectionRule* section_rule, int node_id)
{
    auto node { new TableCombo(*tree_model, node_id, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kRelatedNode), node);

    auto value { new TableDoubleSpin(section_rule->value_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kCredit), value);

    auto subtotal { new TableDoubleSpinR(section_rule->value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kSubtotal), subtotal);

    auto ratio { new TableDoubleSpin(section_rule->ratio_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kRatio), ratio);

    auto date_time { new DateTime(interface_.date_format, section_rule->hide_time, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDateTime), date_time);

    auto line { new Line(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDescription), line);
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kCode), line);

    auto state { new CheckBox(QEvent::MouseButtonRelease, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kState), state);

    auto document { new TableDbClick(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);
}

void MainWindow::DelegateStakeholder(QTableView* view, CSectionRule* section_rule)
{
    auto node { new TableCombo(*product_tree_.model, 0, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kRhsNode), node);

    auto unit_price { new TableDoubleSpin(section_rule->ratio_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kLhsRatio), unit_price);

    auto date_time { new DateTime(interface_.date_format, section_rule->hide_time, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kDateTime), date_time);

    auto line { new Line(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kDescription), line);
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kCode), line);

    auto document { new TableDbClick(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);

    auto state { new CheckBox(QEvent::MouseButtonRelease, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kState), state);
}

void MainWindow::CreateSection(Tree& tree, CString& name, SectionData* data, TableHash* table_hash, CSectionRule* section_rule)
{
    const auto* info { &data->info };
    auto tab_widget { ui->tabWidget };

    auto widget { tree.widget };
    auto view { widget->View() };
    auto model { tree.model };

    CreateDelegate(view, info, section_rule);
    SetConnect(view, widget, model, data->sql.data());

    tab_widget->tabBar()->setTabData(tab_widget->addTab(widget, name), QVariant::fromValue(Tab { info->section, 0 }));

    RestoreState(view->header(), exclusive_interface_, info->node, HEADER_STATE);
    RestoreTableWidget(data, model, section_rule, table_hash, VIEW);

    SetView(view);
    // view->setColumnHidden(1, true);
}

void MainWindow::CreateDelegate(QTreeView* view, CInfo* info, CSectionRule* section_rule)
{
    DelegateCommon(view, info);

    switch (info->section) {
    case Section::kFinance:
    case Section::kTask:
        DelegateFinance(view, info, section_rule);
        break;
    case Section::kStakeholder:
        DelegateStakeholder(view, section_rule);
        break;
    case Section::kProduct:
        DelegateProduct(view, info, section_rule);
        break;
    case Section::kSales:
    case Section::kPurchase:
        DelegateOrder(view, info, section_rule);
        break;
    default:
        break;
    }
}

void MainWindow::DelegateCommon(QTreeView* view, CInfo* info)
{
    auto line { new Line(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kCode), line);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kDescription), line);

    auto plain_text { new TreePlainText(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kNote), plain_text);

    auto node_rule { new TreeCombo(info->rule_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kNodeRule), node_rule);

    auto branch { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kBranch), branch);

    auto unit { new TreeCombo(info->unit_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kUnit), unit);
}

void MainWindow::DelegateFinance(QTreeView* view, CInfo* info, CSectionRule* section_rule)
{
    auto final_total { new TreeDoubleSpinUnitR(section_rule->value_decimal, section_rule->base_unit, info->unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinanceTask::kFinalTotal), final_total);

    auto initial_total { new FinanceForeign(section_rule->value_decimal, section_rule->base_unit, info->unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinanceTask::kInitialTotal), initial_total);
}

void MainWindow::DelegateProduct(QTreeView* view, CInfo* info, CSectionRule* section_rule)
{
    auto quantity { new TreeDoubleSpinDynamicUnitR(section_rule->value_decimal, info->unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kInitialTotal), quantity);

    auto amount { new TreeDoubleSpinUnitR(section_rule->ratio_decimal, finance_rule_.base_unit, finance_data_.info.unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kFinalTotal), amount);

    auto price { new TreeDoubleSpin(section_rule->ratio_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kUnitPrice), price);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kCommission), price);
}

void MainWindow::DelegateStakeholder(QTreeView* view, CSectionRule* section_rule)
{
    auto payment_period { new TreeSpin(IZERO, IMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kPaymentPeriod), payment_period);

    auto tax_rate { new TreeDoubleSpinPercent(section_rule->ratio_decimal, DZERO, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kTaxRate), tax_rate);

    auto deadline { new TreeSpin(IZERO, THIRTY_ONE, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kDeadline), deadline);
}

void MainWindow::DelegateOrder(QTreeView* view, CInfo* info, CSectionRule* section_rule)
{
    auto total { new OrderTotalR(section_rule->value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kInitialTotal), total);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFinalTotal), total);

    auto second { new TreeDoubleSpinR(section_rule->ratio_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSecond), second);

    auto first { new TreeSpinR(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFirst), first);

    auto discount { new TreeDoubleSpinR(section_rule->value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDiscount), discount);

    auto employee { new OrderStakeholder(*stakeholder_tree_.model, UNIT_EMPLOYEE, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kEmployee), employee);

    auto unit_party { info->section == Section::kSales ? UNIT_CUSTOMER : UNIT_VENDOR };
    auto party { new OrderStakeholder(*stakeholder_tree_.model, unit_party, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kParty), party);

    auto date_time { new OrderDateTime(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDateTime), date_time);

    auto locked { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kLocked), locked);
}

void MainWindow::SetConnect(const QTreeView* view, const TreeWidget* widget, const TreeModel* model, const Sqlite* table_sql)
{
    connect(view, &QTreeView::doubleClicked, this, &MainWindow::RTreeViewDoubleClicked);
    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::RTreeViewCustomContextMenuRequested);

    connect(model, &TreeModel::SUpdateName, this, &MainWindow::RUpdateName);

    connect(model, &TreeModel::SUpdateDSpinBox, widget, &TreeWidget::RUpdateDSpinBox);

    connect(model, &TreeModel::SResizeColumnToContents, view, &QTreeView::resizeColumnToContents);

    connect(table_sql, &Sqlite::SRemoveNode, model, &TreeModel::RRemoveNode);
    connect(table_sql, &Sqlite::SUpdateMultiNodeTotal, model, &TreeModel::RUpdateMultiNodeTotal);

    connect(table_sql, &Sqlite::SFreeView, this, &MainWindow::RFreeView);
}

void MainWindow::PrepInsertNode(QTreeView* view)
{
    auto current_index { view->currentIndex() };
    current_index = current_index.isValid() ? current_index : QModelIndex();

    auto parent_index { current_index.parent() };
    parent_index = parent_index.isValid() ? parent_index : QModelIndex();

    InsertNode(parent_index, current_index.row() + 1);
}

void MainWindow::AppendTrans(QWidget* widget)
{
    auto view { GetQTableView(widget) };
    auto model { GetTableModel(view) };
    int row { model->GetRow(0) };
    QModelIndex index {};

    index = (row == -1) ? (model->InsertTrans() ? model->index(model->rowCount() - 1, std::to_underlying(TableEnum::kDateTime)) : index)
                        : model->index(row, std::to_underlying(TableEnum::kRelatedNode));

    view->setCurrentIndex(index);
}

void MainWindow::RInsertTriggered()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget)
        return;

    if (IsTreeWidget(current_widget))
        PrepInsertNode(section_tree_->widget->View());

    if (IsTableWidget(current_widget))
        AppendTrans(current_widget);
}

void MainWindow::RRemoveTriggered()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget)
        return;

    if (IsTreeWidget(current_widget))
        RemoveNode(section_tree_->widget->View(), section_tree_->model);

    if (IsTableWidget(current_widget))
        DeleteTrans(current_widget);
}

void MainWindow::RemoveNode(QTreeView* view, TreeModel* model)
{
    if (!HasSelection(view))
        return;

    auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    auto branch { index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toInt() };

    if (branch) {
        if (model->ChildrenEmpty(node_id))
            model->RemoveNode(index.row(), index.parent());
        else
            RemoveBranch(model, index, node_id);

        return;
    }

    auto& sql { section_data_->sql };
    bool interal_references { sql->InternalReference(node_id) };
    bool exteral_references { sql->ExternalReference(node_id) };

    if (!interal_references && !exteral_references) {
        RemoveView(model, index, node_id);
        return;
    }

    if (interal_references && !exteral_references) {
        auto dialog { new class RemoveNode(*model, node_id, this) };
        connect(dialog, &RemoveNode::SRemoveNode, section_data_->sql.data(), &Sqlite::RRemoveNode);
        connect(dialog, &RemoveNode::SReplaceNode, section_data_->sql.data(), &Sqlite::RReplaceNode);
        dialog->exec();
        return;
    }

    auto dialog { new class RemoveNode(*model, node_id, this) };
    connect(dialog, &RemoveNode::SReplaceNode, section_data_->sql.data(), &Sqlite::RReplaceNode);
    dialog->DisableRemove();
    dialog->exec();
}

void MainWindow::DeleteTrans(QWidget* widget)
{
    auto view { GetQTableView(widget) };
    if (!HasSelection(view))
        return;

    auto model { GetTableModel(view) };
    auto index { view->currentIndex() };

    if (model && index.isValid()) {
        int row { index.row() };
        model->RemoveTrans(row);

        auto row_count { model->rowCount() };
        if (row_count == 0)
            return;

        if (row <= row_count - 1)
            index = model->index(row, 0);
        else if (row_count >= 1)
            index = model->index(row - 1, 0);

        if (index.isValid())
            view->setCurrentIndex(index);
    }
}

void MainWindow::RemoveView(TreeModel* model, const QModelIndex& index, int node_id)
{
    model->RemoveNode(index.row(), index.parent());
    auto widget { section_table_->value(node_id) };

    if (widget) {
        FreeWidget(widget);
        section_table_->remove(node_id);
        SignalStation::Instance().DeregisterModel(section_data_->info.section, node_id);
    }
}

void MainWindow::SaveTableWidget(const TableHash& table_hash, CString& section_name, CString& property)
{
    auto keys { table_hash.keys() };
    QStringList list {};

    for (const auto& node_id : keys)
        list.emplaceBack(QString::number(node_id));

    exclusive_interface_->setValue(QString("%1/%2").arg(section_name, property), list);
}

void MainWindow::RestoreTableWidget(SectionData* data, TreeModel* tree_model, CSectionRule* section_rule, TableHash* table_hash, CString& property)
{
    auto variant { exclusive_interface_->value(QString("%1/%2").arg(data->info.node, property)) };

    QSet<int> list {};

    if (variant.isValid() && variant.canConvert<QStringList>()) {
        auto variant_list { variant.value<QStringList>() };
        for (const auto& node_id : variant_list)
            list.insert(node_id.toInt());
    }

    for (const auto& node_id : list) {
        if (tree_model->Contains(node_id) && !tree_model->Branch(node_id) && node_id >= 1)
            CreateTable(data, tree_model, section_rule, table_hash, node_id);
    }
}

void MainWindow::Recent()
{
    recent_list_ = shared_interface_->value(RECENT_FILE).toStringList();

    auto recent_menu { ui->menuRecent };
    QStringList valid_list {};

    for (const auto& file : recent_list_) {
        if (QFile::exists(file)) {
            auto action { recent_menu->addAction(file) };
            connect(action, &QAction::triggered, this, [file, this]() { ROpenFile(file); });
            valid_list.emplaceBack(file);
        }
    }

    if (recent_list_ != valid_list) {
        recent_list_ = valid_list;
        UpdateRecent();
    }

    SetClearMenuAction();
}

void MainWindow::RemoveBranch(TreeModel* model, const QModelIndex& index, int node_id)
{
    QMessageBox msg {};
    msg.setIcon(QMessageBox::Question);
    msg.setText(tr("Remove %1").arg(model->GetPath(node_id)));
    msg.setInformativeText(tr("The branch will be removed, and its direct children will be promoted to the same level."));
    msg.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

    if (msg.exec() == QMessageBox::Ok)
        model->RemoveNode(index.row(), index.parent());
}

void MainWindow::RTabCloseRequested(int index)
{
    if (index == 0)
        return;

    int node_id { ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id };
    auto widget { section_table_->value(node_id) };

    FreeWidget(widget);
    section_table_->remove(node_id);

    SignalStation::Instance().DeregisterModel(section_data_->info.section, node_id);
}

void MainWindow::SetTabWidget()
{
    auto tab_widget { ui->tabWidget };
    auto tab_bar { tab_widget->tabBar() };

    tab_bar->setDocumentMode(true);
    tab_bar->setExpanding(false);
    tab_bar->setTabButton(0, QTabBar::LeftSide, nullptr);

    tab_widget->setMovable(true);
    tab_widget->setTabsClosable(true);
    tab_widget->setElideMode(Qt::ElideNone);

    start_ = Section(shared_interface_->value(START_SECTION, 0).toInt());

    switch (start_) {
    case Section::kFinance:
        ui->rBtnFinance->setChecked(true);
        break;
    case Section::kStakeholder:
        ui->rBtnStakeholder->setChecked(true);
        break;
    case Section::kProduct:
        ui->rBtnProduct->setChecked(true);
        break;
    case Section::kTask:
        ui->rBtnTask->setChecked(true);
        break;
    case Section::kSales:
        ui->rBtnSales->setChecked(true);
        break;
    case Section::kPurchase:
        ui->rBtnPurchase->setChecked(true);
        break;
    default:
        break;
    }
}

void MainWindow::SetView(QTableView* view)
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::CurrentChanged);
    view->setColumnHidden(std::to_underlying(TableEnum::kID), true);

    auto h_header { view->horizontalHeader() };
    h_header->setSectionResizeMode(QHeaderView::ResizeToContents);
    h_header->setSectionResizeMode(std::to_underlying(TableEnum::kDescription), QHeaderView::Stretch);

    auto v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(ROW_HEIGHT);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);

    view->scrollToBottom();
    view->setCurrentIndex(QModelIndex());
    view->sortByColumn(std::to_underlying(TableEnum::kDateTime), Qt::AscendingOrder); // will run function: AccumulateSubtotal while sorting
}

void MainWindow::SetConnect()
{
    connect(ui->actionInsert, &QAction::triggered, this, &MainWindow::RInsertTriggered);
    connect(ui->actionRemove, &QAction::triggered, this, &MainWindow::RRemoveTriggered);
    connect(ui->actionAppend, &QAction::triggered, this, &MainWindow::RPrepAppendTriggered);
    connect(ui->actionJump, &QAction::triggered, this, &MainWindow::RJumpTriggered);
    connect(ui->actionSearch, &QAction::triggered, this, &MainWindow::RSearchTriggered);
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::RPreferencesTriggered);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::RAboutTriggered);
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::RNewTriggered);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::ROpenTriggered);
    connect(ui->actionClearMenu, &QAction::triggered, this, &MainWindow::RClearMenuTriggered);

    connect(ui->actionDocument, &QAction::triggered, this, &MainWindow::REditDocument);
    connect(ui->actionNode, &QAction::triggered, this, &MainWindow::REditNode);

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::RTabCloseRequested);
    connect(ui->tabWidget, &QTabWidget::tabBarDoubleClicked, this, &MainWindow::RTabBarDoubleClicked);

    connect(ui->actionCheckAll, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(ui->actionCheckNone, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(ui->actionCheckReverse, &QAction::triggered, this, &MainWindow::RUpdateState);
}

void MainWindow::SetFinanceData()
{
    auto section { Section::kFinance };
    auto& info { finance_data_.info };
    auto& sql { finance_data_.sql };
    auto& model { finance_tree_.model };

    info.section = section;
    info.node = FINANCE;
    info.path = FINANCE_PATH;
    info.transaction = FINANCE_TRANSACTION;

    QStringList unit_list { "CNY", "HKD", "USD", "GBP", "JPY", "CAD", "AUD", "EUR" };
    auto& unit_hash { info.unit_hash };

    QStringList unit_symbol_list { "¥", "$", "$", "£", "¥", "$", "$", "€" };
    auto& unit_symbol_hash { info.unit_symbol_hash };

    for (int i = 0; i != unit_list.size(); ++i) {
        unit_hash.insert(i, unit_list.at(i));
        unit_symbol_hash.insert(i, unit_symbol_list.at(i));
    }

    sql_.QuerySectionRule(finance_rule_, section);

    sql = QSharedPointer<SqliteFinance>::create(info);
    finance_data_.search_sql = QSharedPointer<SearchSqlite>::create(info, sql->TransHash());

    model = new TreeModelFinanceTask(sql, info, finance_rule_.base_unit, finance_table_, interface_.separator, this);
    finance_tree_.widget = new TreeWidgetCommon(model, info, finance_rule_, this);
}

void MainWindow::SetProductData()
{
    auto section { Section::kProduct };
    auto& info { product_data_.info };
    auto& sql { product_data_.sql };
    auto& model { product_tree_.model };

    info.section = section;
    info.node = PRODUCT;
    info.path = PRODUCT_PATH;
    info.transaction = PRODUCT_TRANSACTION;

    QStringList unit_list { "", "BX", "PCS", "SET", "SF" };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySectionRule(product_rule_, section);

    sql = QSharedPointer<SqliteProduct>::create(info);
    product_data_.search_sql = QSharedPointer<SearchSqlite>::create(info, sql->TransHash());

    model = new TreeModelProduct(sql, info, product_rule_.base_unit, product_table_, interface_.separator, this);
    product_tree_.widget = new TreeWidgetCommon(model, info, product_rule_, this);
}

void MainWindow::SetStakeholderData()
{
    auto section { Section::kStakeholder };
    auto& info { stakeholder_data_.info };
    auto& sql { stakeholder_data_.sql };
    auto& model { stakeholder_tree_.model };

    info.section = section;
    info.node = STAKEHOLDER;
    info.path = STAKEHOLDER_PATH;
    info.transaction = STAKEHOLDER_TRANSACTION;

    info.rule_hash.insert(0, tr("Cash"));
    info.rule_hash.insert(1, tr("Monthly"));

    QStringList unit_list { "E", "C", "V", "P" };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySectionRule(stakeholder_rule_, section);

    sql = QSharedPointer<SqliteStakeholder>::create(info);
    stakeholder_data_.search_sql = QSharedPointer<SearchSqlite>::create(info, sql->TransHash());

    stakeholder_tree_.model = new TreeModelStakeholder(sql, info, stakeholder_rule_.base_unit, stakeholder_table_, interface_.separator, this);
    stakeholder_tree_.widget = new TreeWidgetStakeholder(model, info, stakeholder_rule_, this);

    connect(product_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
}

void MainWindow::SetTaskData()
{
    auto section { Section::kTask };
    auto& info { task_data_.info };
    auto& sql { task_data_.sql };
    auto& model { task_tree_.model };

    info.section = section;
    info.node = TASK;
    info.path = TASK_PATH;
    info.transaction = TASK_TRANSACTION;

    QStringList unit_list { "", "BX", "PCS", "PER", "SET", "SF" };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySectionRule(task_rule_, section);

    sql = QSharedPointer<SqliteTask>::create(info);
    task_data_.search_sql = QSharedPointer<SearchSqlite>::create(info, sql->TransHash());

    model = new TreeModelFinanceTask(sql, info, task_rule_.base_unit, task_table_, interface_.separator, this);
    task_tree_.widget = new TreeWidgetCommon(model, info, task_rule_, this);
}

void MainWindow::SetSalesData()
{
    auto section { Section::kSales };
    auto& info { sales_data_.info };
    auto& sql { sales_data_.sql };
    auto& model { sales_tree_.model };

    info.section = section;
    info.node = SALES;
    info.path = SALES_PATH;
    info.transaction = SALES_TRANSACTION;

    info.rule_hash.insert(0, tr("Charge"));
    info.rule_hash.insert(1, tr("Refund"));

    QStringList unit_list { tr("Cash"), tr("Monthly"), tr("Pending") };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySectionRule(sales_rule_, section);

    sql = QSharedPointer<SqliteSales>::create(info);
    task_data_.search_sql = QSharedPointer<SearchSqlite>::create(info, sql->TransHash());

    model = new TreeModelOrder(sql, info, sales_rule_.base_unit, sales_table_, interface_.separator, this);
    sales_tree_.widget = new TreeWidgetOrder(model, info, sales_rule_, this);

    connect(product_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
    connect(stakeholder_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
}

void MainWindow::SetPurchaseData()
{
    auto section { Section::kPurchase };
    auto& info { purchase_data_.info };
    auto& sql { purchase_data_.sql };
    auto& model { purchase_tree_.model };

    info.section = section;
    info.node = PURCHASE;
    info.path = PURCHASE_PATH;
    info.transaction = PURCHASE_TRANSACTION;

    info.rule_hash.insert(0, tr("Charge"));
    info.rule_hash.insert(1, tr("Refund"));

    QStringList unit_list { tr("Cash"), tr("Monthly"), tr("Pending") };

    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySectionRule(purchase_rule_, section);

    sql = QSharedPointer<SqlitePurchase>::create(info);
    task_data_.search_sql = QSharedPointer<SearchSqlite>::create(info, sql->TransHash());

    model = new TreeModelOrder(sql, info, purchase_rule_.base_unit, purchase_table_, interface_.separator, this);
    purchase_tree_.widget = new TreeWidgetOrder(model, info, purchase_rule_, this);

    connect(product_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
    connect(stakeholder_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
}

void MainWindow::SetDateFormat() { date_format_list_.emplaceBack(DATE_TIME_FST); }

void MainWindow::SetHeader()
{
    finance_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("NodeRule"), tr("Branch"), tr("Unit"),
        tr("Foreign Total"), tr("Local Total"), "" };
    finance_data_.info.part_table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("FXRate"), tr("Description"), tr("D"), tr("S"), tr("RelatedNode"),
        tr("Debit"), tr("Credit"), tr("Subtotal") };
    finance_data_.info.table_header = {
        tr("ID"),
        tr("DateTime"),
        tr("Code"),
        tr("LhsNode"),
        tr("LhsFXRate"),
        tr("LhsDebit"),
        tr("LhsCredit"),
        tr("Description"),
        tr("D"),
        tr("S"),
        tr("RhsCredit"),
        tr("RhsDebit"),
        tr("RhsFXRate"),
        tr("RhsNode"),
    };

    product_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("NodeRule"), tr("Branch"), tr("Unit"),
        tr("Commission"), tr("UnitPrice"), tr("Quantity Total"), tr("Amount Total"), "" };
    product_data_.info.part_table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("UnitCost"), tr("Description"), tr("D"), tr("S"), tr("RelatedNode"),
        tr("Debit"), tr("Credit"), tr("Subtotal") };
    product_data_.info.table_header = {
        tr("ID"),
        tr("DateTime"),
        tr("Code"),
        tr("LhsNode"),
        tr("LhsUnitCost"),
        tr("LhsDebit"),
        tr("LhsCredit"),
        tr("Description"),
        tr("D"),
        tr("S"),
        tr("RhsCredit"),
        tr("RhsDebit"),
        tr("RhsUnitCost"),
        tr("RhsNode"),
    };

    stakeholder_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Term"), tr("Branch"), tr("Mark"),
        tr("Deadline"), tr("Employee"), tr("PaymentPeriod"), tr("TaxRate"), "" };
    stakeholder_data_.info.part_table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("UnitPrice"), tr("Description"), tr("D"), tr("S"), tr("Inside") };
    stakeholder_data_.info.table_header = {
        tr("ID"),
        tr("DateTime"),
        tr("Code"),
        tr("LhsNode"),
        tr("LhsRatio"),
        tr("LhsDebit"),
        tr("LhsCredit"),
        tr("Description"),
        tr("D"),
        tr("S"),
        tr("RhsCredit"),
        tr("RhsDebit"),
        tr("RhsRatio"),
        tr("RhsNode"),
    };

    task_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("NodeRule"), tr("Branch"), tr("Unit"),
        tr("Quantity Total"), tr("Amount Total"), "" };
    task_data_.info.part_table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("UnitCost"), tr("Description"), tr("D"), tr("S"), tr("RelatedNode"),
        tr("Debit"), tr("Credit"), tr("Subtotal") };
    task_data_.info.table_header = {
        tr("ID"),
        tr("DateTime"),
        tr("Code"),
        tr("LhsNode"),
        tr("LhsUnitCost"),
        tr("LhsDebit"),
        tr("LhsCredit"),
        tr("Description"),
        tr("D"),
        tr("S"),
        tr("RhsCredit"),
        tr("RhsDebit"),
        tr("RhsUnitCost"),
        tr("RhsNode"),
    };

    sales_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("NodeRule"), tr("Branch"), tr("Unit"), tr("Party"),
        tr("Employee"), tr("DateTime"), tr("First"), tr("Second"), tr("Discount"), tr("Locked"), tr("Initial Total"), tr("Final Total") };

    purchase_data_.info.tree_header = sales_data_.info.tree_header;
}

void MainWindow::SetAction()
{
    ui->actionInsert->setIcon(QIcon(":/solarized_dark/solarized_dark/insert.png"));
    ui->actionNode->setIcon(QIcon(":/solarized_dark/solarized_dark/edit.png"));
    ui->actionDocument->setIcon(QIcon(":/solarized_dark/solarized_dark/edit2.png"));
    ui->actionTransport->setIcon(QIcon(":/solarized_dark/solarized_dark/edit2.png"));
    ui->actionRemove->setIcon(QIcon(":/solarized_dark/solarized_dark/remove2.png"));
    ui->actionAbout->setIcon(QIcon(":/solarized_dark/solarized_dark/about.png"));
    ui->actionAppend->setIcon(QIcon(":/solarized_dark/solarized_dark/append.png"));
    ui->actionJump->setIcon(QIcon(":/solarized_dark/solarized_dark/jump.png"));
    ui->actionLocate->setIcon(QIcon(":/solarized_dark/solarized_dark/locate.png"));
    ui->actionPreferences->setIcon(QIcon(":/solarized_dark/solarized_dark/settings.png"));
    ui->actionSearch->setIcon(QIcon(":/solarized_dark/solarized_dark/search.png"));
    ui->actionNew->setIcon(QIcon(":/solarized_dark/solarized_dark/new.png"));
    ui->actionOpen->setIcon(QIcon(":/solarized_dark/solarized_dark/open.png"));
    ui->actionCheckAll->setIcon(QIcon(":/solarized_dark/solarized_dark/check-all.png"));
    ui->actionCheckNone->setIcon(QIcon(":/solarized_dark/solarized_dark/check-none.png"));
    ui->actionCheckReverse->setIcon(QIcon(":/solarized_dark/solarized_dark/check-reverse.png"));

    ui->actionCheckAll->setProperty(CHECK, std::to_underlying(Check::kAll));
    ui->actionCheckNone->setProperty(CHECK, std::to_underlying(Check::kNone));
    ui->actionCheckReverse->setProperty(CHECK, std::to_underlying(Check::kReverse));
}

void MainWindow::SetView(QTreeView* view)
{
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setDragDropMode(QAbstractItemView::InternalMove);
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    view->setDropIndicatorShown(true);
    view->setSortingEnabled(true);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setExpandsOnDoubleClick(true);

    auto header { view->header() };
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
    header->setStretchLastSection(true);
    header->setDefaultAlignment(Qt::AlignCenter);
}

void MainWindow::RPrepAppendTriggered()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTreeWidget(current_widget))
        return;

    auto view { section_tree_->widget->View() };
    if (!HasSelection(view))
        return;

    auto parent_index { view->currentIndex() };
    if (!parent_index.isValid())
        return;

    bool branch { parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool() };
    if (!branch)
        return;

    InsertNode(parent_index, 0);
}

void MainWindow::RJumpTriggered()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTableWidget(current_widget))
        return;

    auto view { GetQTableView(current_widget) };
    if (!HasSelection(view))
        return;

    auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    int row { index.row() };
    auto related_node_id { index.sibling(row, std::to_underlying(TableEnum::kRelatedNode)).data().toInt() };
    if (related_node_id == 0)
        return;

    if (!section_table_->contains(related_node_id))
        CreateTable(section_data_, section_tree_->model, section_rule_, section_table_, related_node_id);

    auto trans_id { index.sibling(row, std::to_underlying(TableEnum::kID)).data().toInt() };
    SwitchTab(related_node_id, trans_id);
}

void MainWindow::RTreeViewCustomContextMenuRequested(const QPoint& pos)
{
    Q_UNUSED(pos);

    auto menu = new QMenu(this);
    menu->addAction(ui->actionInsert);
    menu->addAction(ui->actionNode);
    menu->addAction(ui->actionAppend);
    menu->addAction(ui->actionRemove);

    menu->exec(QCursor::pos());
}

void MainWindow::REditNode()
{
    const auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTreeWidget(current_widget))
        return;

    const auto view { section_tree_->widget->View() };
    if (!HasSelection(view))
        return;

    const auto& index { view->currentIndex() };
    if (!index.isValid())
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    auto model { section_tree_->model };

    const auto& parent_index { index.parent() };
    const int parent_id { parent_index.isValid() ? parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() : -1 };
    auto parent_path { model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto name_list { model->ChildrenName(parent_id, node_id) };

    auto tmp_node { ResourcePool<Node>::Instance().Allocate() };
    model->CopyNode(tmp_node, node_id);

    bool enable_branch { !section_data_->sql->InternalReference(node_id) && !section_data_->sql->ExternalReference(node_id)
        && !section_table_->contains(node_id) && model->ChildrenEmpty(node_id) };

    const auto& info { section_data_->info };

    QDialog* dialog {};
    auto section { info.section };

    switch (section) {
    case Section::kFinance:
        dialog = new EditNodeFinance(tmp_node, info.unit_hash, parent_path, name_list, enable_branch, this);
        break;
    case Section::kTask:
        dialog = new EditNodeFinance(tmp_node, info.unit_hash, parent_path, name_list, enable_branch, this);
        break;
    case Section::kStakeholder:
        dialog = new EditNodeStakeholder(tmp_node, info.unit_hash, parent_path, name_list, enable_branch, section_rule_->ratio_decimal, model, this);
        break;
    case Section::kProduct:
        dialog = new EditNodeProduct(tmp_node, info.unit_hash, parent_path, name_list, enable_branch, section_rule_->ratio_decimal, this);
        break;
    case Section::kSales:
        dialog = new EditNodeOrder(tmp_node, model, stakeholder_tree_.model, *product_tree_.model, sales_rule_.value_decimal, UNIT_CUSTOMER, this);
        dialog->setWindowTitle(tr(Sales));
        break;
    case Section::kPurchase:
        dialog = new EditNodeOrder(tmp_node, model, stakeholder_tree_.model, *product_tree_.model, sales_rule_.value_decimal, UNIT_VENDOR, this);
        dialog->setWindowTitle(tr(Purchase));
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(tmp_node);
    }

    connect(dialog, &QDialog::rejected, this, [=, this]() {
        section_dialog_->removeOne(dialog);
        ResourcePool<Node>::Instance().Recycle(tmp_node);
    });

    if (section != Section::kPurchase && section != Section::kSales) {
        connect(dialog, &QDialog::accepted, this, [=]() {
            model->UpdateNode(tmp_node);
            ResourcePool<Node>::Instance().Recycle(tmp_node);
        });

        dialog->exec();
    }

    if (section == Section::kPurchase || section == Section::kSales) {
        connect(stakeholder_tree_.model, &TreeModelStakeholder::SUpdateOrderPartyEmployee, dynamic_cast<EditNodeOrder*>(dialog),
            &EditNodeOrder::RUpdateStakeholder);

        connect(model, &TreeModel::SUpdateOrder, dynamic_cast<EditNodeOrder*>(dialog), &EditNodeOrder::RUpdateOrder);
        section_dialog_->append(dialog);

        dialog->show();
    }
}

void MainWindow::InsertNode(const QModelIndex& parent, int row)
{
    auto model { section_tree_->model };

    const int parent_id { parent.isValid() ? parent.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() : -1 };
    auto parent_path { model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->node_rule = model->NodeRule(parent_id);
    node->unit = model->Unit(parent_id);
    model->SetParent(node, parent_id);

    const auto& info { section_data_->info };
    const auto& section { info.section };
    const auto name_list { model->ChildrenName(parent_id, 0) };

    QDialog* dialog {};

    switch (section) {
    case Section::kFinance:
        dialog = new EditNodeFinance(node, info.unit_hash, parent_path, name_list, true, this);
        break;
    case Section::kTask:
        dialog = new EditNodeFinance(node, info.unit_hash, parent_path, name_list, true, this);
        break;
    case Section::kStakeholder:
        dialog = new EditNodeStakeholder(node, info.unit_hash, parent_path, name_list, true, section_rule_->ratio_decimal, model, this);
        break;
    case Section::kProduct:
        dialog = new EditNodeProduct(node, info.unit_hash, parent_path, name_list, true, section_rule_->ratio_decimal, this);
        break;
    case Section::kSales:
        dialog = new InsertNodeOrder(node, model, stakeholder_tree_.model, *product_tree_.model, purchase_rule_.value_decimal, UNIT_CUSTOMER, this);
        dialog->setWindowTitle(tr(Sales));
        break;
    case Section::kPurchase:
        dialog = new InsertNodeOrder(node, model, stakeholder_tree_.model, *product_tree_.model, purchase_rule_.value_decimal, UNIT_VENDOR, this);
        dialog->setWindowTitle(tr(Purchase));
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(node);
    }

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (model->InsertNode(row, parent, node)) {
            auto index = model->index(row, 0, parent);
            section_tree_->widget->SetCurrentIndex(index);
        }
    });

    if (section != Section::kPurchase && section != Section::kSales) {
        connect(dialog, &QDialog::rejected, this, [=, this]() {
            ResourcePool<Node>::Instance().Recycle(node);
            section_dialog_->removeOne(dialog);
        });

        dialog->exec();
    }

    if (section == Section::kPurchase || section == Section::kSales) {
        connect(stakeholder_tree_.model, &TreeModelStakeholder::SUpdateOrderPartyEmployee, dynamic_cast<InsertNodeOrder*>(dialog),
            &InsertNodeOrder::RUpdateStakeholder);

        connect(model, &TreeModel::SUpdateOrder, dynamic_cast<EditNodeOrder*>(dialog), &EditNodeOrder::RUpdateOrder);
        connect(dialog, &QDialog::rejected, this, [=, this]() { section_dialog_->removeOne(dialog); });

        section_dialog_->append(dialog);
        dialog->show();
    }
}

void MainWindow::REditDocument()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTableWidget(current_widget))
        return;

    auto view { GetQTableView(current_widget) };
    if (!HasSelection(view))
        return;

    auto trans_index { view->currentIndex() };
    if (!trans_index.isValid())
        return;

    auto document_dir { section_rule_->document_dir };
    auto model { GetTableModel(view) };
    auto document_pointer { model->GetDocumentPointer(trans_index) };
    auto trans_id { trans_index.siblingAtColumn(std::to_underlying(TableEnum::kID)).data().toInt() };

    auto dialog { new EditDocument(section_data_->info.section, document_pointer, document_dir, this) };
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    if (dialog->exec() == QDialog::Accepted)
        section_data_->sql->UpdateField(section_data_->info.transaction, document_pointer->join(SEMICOLON), DOCUMENT, trans_id);
}

void MainWindow::RUpdateName(const Node* node)
{
    auto model { section_tree_->model };
    int node_id { node->id };
    auto tab_bar { ui->tabWidget->tabBar() };
    int count { ui->tabWidget->count() };

    if (!node->branch) {
        if (!section_table_->contains(node_id))
            return;

        auto path { model->GetPath(node_id) };

        for (int index = 0; index != count; ++index) {
            if (tab_bar->tabData(index).value<Tab>().node_id == node_id) {
                tab_bar->setTabText(index, node->name);
                tab_bar->setTabToolTip(index, path);
            }
        }
    }

    if (node->branch) {
        QQueue<const Node*> queue {};
        queue.enqueue(node);

        QList<int> list {};
        while (!queue.isEmpty()) {
            auto queue_node = queue.dequeue();

            if (queue_node->branch)
                for (const auto& child : queue_node->children)
                    queue.enqueue(child);
            else
                list.emplaceBack(queue_node->id);
        }

        int node_id {};
        QString path {};

        for (int index = 0; index != count; ++index) {
            node_id = tab_bar->tabData(index).value<Tab>().node_id;

            if (list.contains(node_id)) {
                path = model->GetPath(node_id);
                tab_bar->setTabToolTip(index, path);
            }
        }
    }
}

void MainWindow::RUpdateSettings(CSectionRule& section_rule, const Interface& interface)
{
    if (interface_ != interface)
        UpdateInterface(interface);

    if (*section_rule_ != section_rule) {
        bool update_base_unit { section_rule_->base_unit != section_rule.base_unit };
        bool resize_column { section_rule_->value_decimal != section_rule.value_decimal || section_rule_->ratio_decimal != section_rule.ratio_decimal
            || section_rule_->hide_time != section_rule.hide_time };

        *section_rule_ = section_rule;

        if (update_base_unit)
            section_tree_->model->UpdateBaseUnit(section_rule.base_unit);

        section_tree_->widget->SetStatus();
        sql_.UpdateSectionRule(section_rule, section_data_->info.section);

        if (resize_column) {
            auto current_widget { ui->tabWidget->currentWidget() };
            if (IsTableWidget(current_widget))
                ResizeColumn(GetQTableView(current_widget)->horizontalHeader(), true);
            if (IsTreeWidget(current_widget))
                ResizeColumn(section_tree_->widget->Header(), false);
        }
    }
}
void MainWindow::RFreeView(int node_id)
{
    auto view { section_table_->value(node_id) };

    if (view) {
        FreeWidget(view);
        section_table_->remove(node_id);
        SignalStation::Instance().DeregisterModel(section_data_->info.section, node_id);
    }
}

void MainWindow::UpdateInterface(const Interface& interface)
{
    auto new_language { interface.language };
    if (interface_.language != new_language) {
        if (new_language == EN_US) {
            qApp->removeTranslator(&cash_translator_);
            qApp->removeTranslator(&base_translator_);
        } else
            LoadAndInstallTranslator(new_language);

        ui->retranslateUi(this);
        SetHeader();
        UpdateTranslate();
    }

    auto new_separator { interface.separator };
    auto old_separator { interface_.separator };

    if (old_separator != new_separator) {
        finance_tree_.model->UpdateSeparator(old_separator, new_separator);
        stakeholder_tree_.model->UpdateSeparator(old_separator, new_separator);
        product_tree_.model->UpdateSeparator(old_separator, new_separator);
        task_tree_.model->UpdateSeparator(old_separator, new_separator);
        sales_tree_.model->UpdateSeparator(old_separator, new_separator);
        purchase_tree_.model->UpdateSeparator(old_separator, new_separator);

        auto widget { ui->tabWidget };
        int count { ui->tabWidget->count() };

        for (int index = 0; index != count; ++index)
            widget->setTabToolTip(index, widget->tabToolTip(index).replace(old_separator, new_separator));
    }

    interface_ = interface;

    shared_interface_->beginGroup(INTERFACE);
    shared_interface_->setValue(LANGUAGE, interface.language);
    shared_interface_->setValue(SEPARATOR, interface.separator);
    shared_interface_->setValue(DATE_FORMAT, interface.date_format);
    shared_interface_->endGroup();
}

void MainWindow::UpdateTranslate()
{
    QWidget* widget {};
    Tab tab_id {};
    auto tab_widget { ui->tabWidget };
    auto tab_bar { tab_widget->tabBar() };
    int count { tab_widget->count() };

    for (int index = 0; index != count; ++index) {
        widget = tab_widget->widget(index);
        tab_id = tab_bar->tabData(index).value<Tab>();

        if (IsTreeWidget(widget)) {
            Section section { tab_id.section };
            switch (section) {
            case Section::kFinance:
                tab_widget->setTabText(index, tr(Finance));
                break;
            case Section::kStakeholder:
                tab_widget->setTabText(index, tr(Stakeholder));
                break;
            case Section::kProduct:
                tab_widget->setTabText(index, tr(Product));
                break;
            case Section::kTask:
                tab_widget->setTabText(index, tr(Task));
                break;
            default:
                break;
            }
        }
    }
}

void MainWindow::UpdateRecent() { shared_interface_->setValue(RECENT_FILE, recent_list_); }

void MainWindow::LoadAndInstallTranslator(CString& language)
{
    QString cash_language { QString(":/I18N/I18N/") + YTX + "_" + language + SFX_QM };
    if (cash_translator_.load(cash_language))
        qApp->installTranslator(&cash_translator_);

    QString base_language { ":/I18N/I18N/qtbase_" + language + SFX_QM };
    if (base_translator_.load(base_language))
        qApp->installTranslator(&base_translator_);
}

void MainWindow::ResizeColumn(QHeaderView* header, bool table_view)
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_view ? header->setSectionResizeMode(std::to_underlying(TableEnumSearch::kDescription), QHeaderView::Stretch)
               : header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
    ;
}

void MainWindow::SharedInterface(CString& dir_path)
{
    static QSettings shared_interface(dir_path + SLASH + YTX + SFX_INI, QSettings::IniFormat);
    shared_interface_ = &shared_interface;

    shared_interface.beginGroup(INTERFACE);
    interface_.language = shared_interface.value(LANGUAGE, EN_US).toString();
    interface_.theme = shared_interface.value(THEME, SOLARIZED_DARK).toString();
    interface_.date_format = shared_interface.value(DATE_FORMAT, DATE_TIME_FST).toString();
    interface_.separator = shared_interface.value(SEPARATOR, DASH).toString();
    shared_interface.endGroup();

    LoadAndInstallTranslator(interface_.language);

    QString theme { "file:///:/theme/theme/" + interface_.theme + SFX_QSS };
    qApp->setStyleSheet(theme);
}

void MainWindow::ExclusiveInterface(CString& dir_path, CString& base_name)
{
    static QSettings exclusive_interface(dir_path + SLASH + base_name + SFX_INI, QSettings::IniFormat);
    exclusive_interface_ = &exclusive_interface;
}

void MainWindow::ResourceFile()
{
    QString path {};

#ifdef Q_OS_WIN
    path = QCoreApplication::applicationDirPath() + "/resource";

    if (QDir dir(path); !dir.exists()) {
        if (!QDir::home().mkpath(path)) {
            qDebug() << "Failed to create directory:" << path;
            return;
        }
    }

    path += "/resource.brc";

#if 0
    QString command { "D:/Qt/6.7.2/llvm-mingw_64/bin/rcc.exe" };
    QStringList arguments {};
    arguments << "-binary"
              << "D:/YTX/resource/resource.qrc"
              << "-o" << path;

    QProcess process {};

    // 启动终端并执行命令
    process.start(command, arguments);
    process.waitForFinished();
#endif

#elif defined(Q_OS_MACOS)
    path = QCoreApplication::applicationDirPath() + "/../Resources/resource.brc";

#if 0
    QString command { QDir::homePath() + "/Qt/6.7.2/macos/libexec/rcc" + " -binary " + QDir::homePath() + "/Documents/YTX/resource/resource.qrc -o " + path };

    QProcess process {};
    process.start("zsh", QStringList() << "-c" << command);
    process.waitForFinished();
#endif

#endif

    QResource::registerResource(path);
}

void MainWindow::RSearchTriggered()
{
    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    auto dialog { new Search(
        section_data_->info, interface_, *section_tree_->model, section_data_->search_sql, *section_rule_, section_data_->info.rule_hash, this) };
    connect(dialog, &Search::STreeLocation, this, &MainWindow::RTreeLocation);
    connect(dialog, &Search::STableLocation, this, &MainWindow::RTableLocation);
    connect(section_tree_->model, &TreeModel::SSearch, dialog, &Search::RSearch);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    section_dialog_->append(dialog);
    dialog->show();
}

void MainWindow::RTreeLocation(int node_id)
{
    auto widget { section_tree_->widget };
    ui->tabWidget->setCurrentWidget(widget);

    auto index { section_tree_->model->GetIndex(node_id) };
    widget->activateWindow();
    widget->SetCurrentIndex(index);
}

void MainWindow::RTableLocation(int trans_id, int lhs_node_id, int rhs_node_id)
{
    int id { lhs_node_id };

    auto Contains = [&](int node_id) {
        if (section_table_->contains(node_id)) {
            id = node_id;
            return true;
        }
        return false;
    };

    if (!Contains(lhs_node_id) && !Contains(rhs_node_id))
        CreateTable(section_data_, section_tree_->model, section_rule_, section_table_, id);

    SwitchTab(id, trans_id);
}

void MainWindow::RPreferencesTriggered()
{
    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    auto model { section_tree_->model };

    auto preference { new Preferences(section_data_->info, *model, date_format_list_, interface_, *section_rule_, this) };
    connect(preference, &Preferences::SUpdateSettings, this, &MainWindow::RUpdateSettings);
    preference->exec();
}

void MainWindow::RAboutTriggered()
{
    static About* dialog = nullptr;

    if (!dialog) {
        dialog = new About(this);
        dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &QDialog::finished, [=]() { dialog = nullptr; });
    }

    dialog->show();
    dialog->activateWindow();
}

void MainWindow::RNewTriggered()
{
    QString filter("*.ytx");
    auto file_path { QFileDialog::getSaveFileName(this, tr("New"), QDir::homePath(), filter, nullptr) };
    if (file_path.isEmpty())
        return;

    if (!file_path.endsWith(SFX_YTX, Qt::CaseInsensitive))
        file_path += SFX_YTX;

    sql_.NewFile(file_path);
    ROpenFile(file_path);
}

void MainWindow::ROpenTriggered()
{
    QString filter("*.ytx");
    auto file_path { QFileDialog::getOpenFileName(this, tr("Open"), QDir::homePath(), filter, nullptr) };

    ROpenFile(file_path);
}

void MainWindow::SetClearMenuAction()
{
    auto menu { ui->menuRecent };

    if (!menu->isEmpty()) {
        auto separator { ui->actionSeparator };
        menu->addAction(separator);
        separator->setSeparator(true);

        menu->addAction(ui->actionClearMenu);
    }
}

void MainWindow::RClearMenuTriggered()
{
    ui->menuRecent->clear();
    recent_list_.clear();
    UpdateRecent();
}

void MainWindow::RTabBarDoubleClicked(int index) { RTreeLocation(ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id); }

void MainWindow::RUpdateState()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTableWidget(current_widget))
        return;

    GetTableModel(GetQTableView(current_widget))->UpdateAllState(Check { QObject::sender()->property(CHECK).toInt() });
}

void MainWindow::SwitchSection(const Tab& last_tab)
{
    auto tab_widget { ui->tabWidget };
    auto tab_bar { tab_widget->tabBar() };
    int count { tab_widget->count() };
    Tab tab {};

    for (int index = 0; index != count; ++index) {
        tab = tab_bar->tabData(index).value<Tab>();
        tab.section == start_ ? tab_widget->setTabVisible(index, true) : tab_widget->setTabVisible(index, false);

        if (tab == last_tab)
            tab_widget->setCurrentIndex(index);
    }

    SwitchDialog(section_dialog_, true);
}

void MainWindow::SwitchDialog(QList<PDialog>* dialog_list, bool enable)
{
    if (dialog_list) {
        for (auto& dialog : *dialog_list)
            if (dialog)
                dialog->setVisible(enable);
    }
}

void MainWindow::UpdateLastTab()
{
    if (section_data_) {
        auto index { ui->tabWidget->currentIndex() };
        section_data_->last_tab = ui->tabWidget->tabBar()->tabData(index).value<Tab>();
    }
}

void MainWindow::on_rBtnFinance_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kFinance;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(section_dialog_, false);
    UpdateLastTab();

    section_tree_ = &finance_tree_;
    section_table_ = &finance_table_;
    section_dialog_ = &finance_dialog_;
    section_rule_ = &finance_rule_;
    section_data_ = &finance_data_;

    SwitchSection(section_data_->last_tab);
}

void MainWindow::on_rBtnSales_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kSales;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(section_dialog_, false);
    UpdateLastTab();

    section_tree_ = &sales_tree_;
    section_table_ = &sales_table_;
    section_dialog_ = &sales_dialog_;
    section_rule_ = &sales_rule_;
    section_data_ = &sales_data_;

    SwitchSection(section_data_->last_tab);
}

void MainWindow::on_rBtnTask_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kTask;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(section_dialog_, false);
    UpdateLastTab();

    section_tree_ = &task_tree_;
    section_table_ = &task_table_;
    section_dialog_ = &task_dialog_;
    section_rule_ = &task_rule_;
    section_data_ = &task_data_;

    SwitchSection(section_data_->last_tab);
}

void MainWindow::on_rBtnStakeholder_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kStakeholder;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(section_dialog_, false);
    UpdateLastTab();

    section_tree_ = &stakeholder_tree_;
    section_table_ = &stakeholder_table_;
    section_dialog_ = &stakeholder_dialog_;
    section_rule_ = &stakeholder_rule_;
    section_data_ = &stakeholder_data_;

    SwitchSection(section_data_->last_tab);
}

void MainWindow::on_rBtnProduct_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kProduct;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(section_dialog_, false);
    UpdateLastTab();

    section_tree_ = &product_tree_;
    section_table_ = &product_table_;
    section_dialog_ = &product_dialog_;
    section_rule_ = &product_rule_;
    section_data_ = &product_data_;

    SwitchSection(section_data_->last_tab);
}

void MainWindow::on_rBtnPurchase_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kPurchase;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(section_dialog_, false);
    UpdateLastTab();

    section_tree_ = &purchase_tree_;
    section_table_ = &purchase_table_;
    section_dialog_ = &purchase_dialog_;
    section_rule_ = &purchase_rule_;
    section_data_ = &purchase_data_;

    SwitchSection(section_data_->last_tab);
}
