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
#include "delegate/specificunit.h"
#include "delegate/table/colorr.h"
#include "delegate/table/insideproduct.h"
#include "delegate/table/tablecombo.h"
#include "delegate/table/tabledbclick.h"
#include "delegate/table/tabledoublespin.h"
#include "delegate/table/tabledoublespinr.h"
#include "delegate/tree/finance/financeforeign.h"
#include "delegate/tree/order/orderdatetime.h"
#include "delegate/tree/order/ordertotal.h"
#include "delegate/tree/stakeholder/color.h"
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
        SaveTableWidget(finance_table_hash_, finance_data_.info.node, VIEW);
        SaveState(finance_tree_.widget->Header(), exclusive_interface_, finance_data_.info.node, HEADER_STATE);

        finance_dialog_list_.clear();
    }

    if (product_tree_.widget) {
        SaveTableWidget(product_table_hash_, product_data_.info.node, VIEW);
        SaveState(product_tree_.widget->Header(), exclusive_interface_, product_data_.info.node, HEADER_STATE);

        product_dialog_list_.clear();
    }

    if (stakeholder_tree_.widget) {
        SaveTableWidget(stakeholder_table_hash_, stakeholder_data_.info.node, VIEW);
        SaveState(stakeholder_tree_.widget->Header(), exclusive_interface_, stakeholder_data_.info.node, HEADER_STATE);

        stakeholder_dialog_list_.clear();
    }

    if (task_tree_.widget) {
        SaveTableWidget(task_table_hash_, task_data_.info.node, VIEW);
        SaveState(task_tree_.widget->Header(), exclusive_interface_, task_data_.info.node, HEADER_STATE);

        task_dialog_list_.clear();
    }

    if (sales_tree_.widget) {
        SaveTableWidget(sales_table_hash_, sales_data_.info.node, VIEW);
        SaveState(sales_tree_.widget->Header(), exclusive_interface_, sales_data_.info.node, HEADER_STATE);

        sales_dialog_list_.clear();
    }

    if (purchase_tree_.widget) {
        SaveTableWidget(purchase_table_hash_, purchase_data_.info.node, VIEW);
        SaveState(purchase_tree_.widget->Header(), exclusive_interface_, purchase_data_.info.node, HEADER_STATE);

        purchase_dialog_list_.clear();
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

        CreateSection(finance_tree_, tr(Finance), &finance_data_, &finance_table_hash_, &finance_settings_);
        CreateSection(stakeholder_tree_, tr(Stakeholder), &stakeholder_data_, &stakeholder_table_hash_, &stakeholder_settings_);
        CreateSection(product_tree_, tr(Product), &product_data_, &product_table_hash_, &product_settings_);
        CreateSection(task_tree_, tr(Task), &task_data_, &task_table_hash_, &task_settings_);
        CreateSection(sales_tree_, tr(Sales), &sales_data_, &sales_table_hash_, &sales_settings_);
        CreateSection(purchase_tree_, tr(Purchase), &purchase_data_, &purchase_table_hash_, &purchase_settings_);

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

void MainWindow::RInsertTriggered()
{
    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

    if (IsTreeWidget(widget)) {
        assert(dynamic_cast<TreeWidget*>(widget) && "Widget is not TreeWidget");
        InsertNode(static_cast<TreeWidget*>(widget));
    }

    if (IsTableWidget(widget)) {
        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        AppendTrans(static_cast<TableWidget*>(widget));
    }
}

void MainWindow::RTreeViewDoubleClicked(const QModelIndex& index)
{
    if (index.column() != 0)
        return;

    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kBranch)).data().toBool() };
    if (branch)
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() };
    if (node_id == -1)
        return;

    if (!table_hash_->contains(node_id))
        CreateTable(data_, tree_->model, settings_, table_hash_, node_id);

    SwitchTab(node_id);
}

void MainWindow::SwitchTab(int node_id, int trans_id)
{
    auto widget { table_hash_->value(node_id, nullptr) };
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

void MainWindow::CreateTable(Data* data, TreeModel* tree_model, CSettings* settings, TableHash* table_hash, int node_id)
{
    CString& name { tree_model->Name(node_id) };
    auto section { data->info.section };
    auto rule { tree_model->Rule(node_id) };

    auto tmp_node { ResourcePool<Node>::Instance().Allocate() };
    tree_model->CopyNode(tmp_node, node_id);

    TableWidget* widget {};

    switch (section) {
    case Section::kSales:
        widget = new TableWidgetOrder(tmp_node, tree_model, stakeholder_tree_.model, product_tree_.model, settings->value_decimal, UNIT_CUSTOMER, this);
        break;
    case Section::kPurchase:
        widget = new TableWidgetOrder(tmp_node, tree_model, stakeholder_tree_.model, product_tree_.model, settings->value_decimal, UNIT_VENDOR, this);
        break;
    default:
        widget = new TableWidgetCommon(this);
        break;
    }

    TableModel* model {};

    switch (section) {
    case Section::kFinance:
        model = new TableModelFinance(data->sql, rule, node_id, data->info, this);
        break;
    case Section::kProduct:
        model = new TableModelProduct(data->sql, rule, node_id, data->info, this);
        break;
    case Section::kTask:
        model = new TableModelTask(data->sql, rule, node_id, data->info, this);
        break;
    case Section::kStakeholder:
        model = new TableModelStakeholder(data->sql, rule, node_id, data->info, this);
        break;
    case Section::kSales:
        model = new TableModelOrder(data->sql, rule, node_id, data->info, product_tree_.model, this);
        break;
    case Section::kPurchase:
        model = new TableModelOrder(data->sql, rule, node_id, data->info, product_tree_.model, this);
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
            CreateDelegate(view, tree_model, settings, node_id);
            break;
        case Section::kStakeholder:
            DelegateStakeholder(view, settings);
            break;
        default:
            break;
        }
    }

    table_hash->insert(node_id, widget);
    SignalStation::Instance().RegisterModel(section, node_id, model);
}

void MainWindow::SetConnect(const QTableView* view, const TableModel* table, const TreeModel* tree, const Data* data)
{
    connect(table, &TableModel::SResizeColumnToContents, view, &QTableView::resizeColumnToContents);

    connect(table, &TableModel::SRemoveOneTrans, &SignalStation::Instance(), &SignalStation::RRemoveOneTrans);
    connect(table, &TableModel::SAppendOneTrans, &SignalStation::Instance(), &SignalStation::RAppendOneTrans);
    connect(table, &TableModel::SUpdateBalance, &SignalStation::Instance(), &SignalStation::RUpdateBalance);

    connect(table, &TableModel::SUpdateLeafTotal, tree, &TreeModel::RUpdateLeafTotal);
    connect(table, &TableModel::SSearch, tree, &TreeModel::RSearch);

    connect(tree, &TreeModel::SRule, table, &TableModel::RRule);

    connect(data->sql.data(), &Sqlite::SRemoveMultiTrans, table, &TableModel::RRemoveMultiTrans);
    connect(data->sql.data(), &Sqlite::SMoveMultiTrans, table, &TableModel::RMoveMultiTrans);
}

void MainWindow::CreateDelegate(QTableView* view, const TreeModel* tree_model, CSettings* settings, int node_id)
{
    auto node { new TableCombo(tree_model, node_id, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kRhsNode), node);

    auto value { new TableDoubleSpin(settings->value_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kCredit), value);

    auto subtotal { new TableDoubleSpinR(settings->value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kSubtotal), subtotal);

    auto ratio { new TableDoubleSpin(settings->ratio_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnum::kLhsRatio), ratio);

    auto date_time { new DateTime(interface_.date_format, settings->hide_time, view) };
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

void MainWindow::DelegateStakeholder(QTableView* view, CSettings* settings)
{
    auto inside_product { new InsideProduct(product_tree_.model, UNIT_POSITION, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kInsideProduct), inside_product);

    auto unit_price { new TableDoubleSpin(settings->ratio_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kUnitPrice), unit_price);

    auto date_time { new DateTime(interface_.date_format, settings->hide_time, view) };
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

void MainWindow::DelegateOrder(QTableView* view, CSettings* settings)
{
    auto inside_product { new InsideProduct(product_tree_.model, UNIT_POSITION, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kInsideProduct), inside_product);

    auto outside_product { new SpecificUnit(stakeholder_tree_.model, UNIT_PRODUCT, view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kOutsideProduct), outside_product);

    auto color { new ColorR(view) };
    view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kColor), color);
}

void MainWindow::CreateSection(Tree& tree, CString& name, Data* data, TableHash* table_hash, CSettings* settings)
{
    const auto* info { &data->info };
    auto tab_widget { ui->tabWidget };

    auto widget { tree.widget };
    auto view { widget->View() };
    auto model { tree.model };

    CreateDelegate(view, info, settings);
    SetConnect(view, widget, model, data->sql.data());

    tab_widget->tabBar()->setTabData(tab_widget->addTab(widget, name), QVariant::fromValue(Tab { info->section, 0 }));

    RestoreState(view->header(), exclusive_interface_, info->node, HEADER_STATE);
    RestoreTableWidget(data, model, settings, table_hash, VIEW);

    SetView(view);
    // view->setColumnHidden(1, true);
}

void MainWindow::CreateDelegate(QTreeView* view, CInfo* info, CSettings* settings)
{
    DelegateCommon(view, info);

    switch (info->section) {
    case Section::kFinance:
    case Section::kTask:
        DelegateFinance(view, info, settings);
        break;
    case Section::kStakeholder:
        DelegateStakeholder(view, settings);
        break;
    case Section::kProduct:
        DelegateProduct(view, info, settings);
        break;
    case Section::kSales:
    case Section::kPurchase:
        DelegateOrder(view, info, settings);
        break;
    default:
        break;
    }
}

void MainWindow::DelegateCommon(QTreeView* view, CInfo* info)
{
    auto line { new Line(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumCommon::kCode), line);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumCommon::kDescription), line);

    auto plain_text { new TreePlainText(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumCommon::kNote), plain_text);

    auto rule { new TreeCombo(info->rule_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumCommon::kRule), rule);

    auto branch { new CheckBox(QEvent::MouseButtonDblClick, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumCommon::kBranch), branch);

    auto unit { new TreeCombo(info->unit_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumCommon::kUnit), unit);
}

void MainWindow::DelegateFinance(QTreeView* view, CInfo* info, CSettings* settings)
{
    auto final_total { new TreeDoubleSpinUnitR(settings->value_decimal, settings->base_unit, info->unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kFinalTotal), final_total);

    auto initial_total { new FinanceForeign(settings->value_decimal, settings->base_unit, info->unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kInitialTotal), initial_total);
}

void MainWindow::DelegateProduct(QTreeView* view, CInfo* info, CSettings* settings)
{
    auto quantity { new TreeDoubleSpinDynamicUnitR(settings->value_decimal, info->unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kInitialTotal), quantity);

    auto amount { new TreeDoubleSpinUnitR(settings->ratio_decimal, finance_settings_.base_unit, finance_data_.info.unit_symbol_hash, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kFinalTotal), amount);

    auto price { new TreeDoubleSpin(settings->ratio_decimal, DMIN, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kUnitPrice), price);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kCommission), price);

    auto color { new Color(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kColor), color);
}

void MainWindow::DelegateStakeholder(QTreeView* view, CSettings* settings)
{
    auto payment_period { new TreeSpin(IZERO, IMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kPaymentPeriod), payment_period);

    auto tax_rate { new TreeDoubleSpinPercent(settings->ratio_decimal, DZERO, DMAX, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kTaxRate), tax_rate);

    auto deadline { new TreeSpin(IZERO, THIRTY_ONE, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kDeadline), deadline);
}

void MainWindow::DelegateOrder(QTreeView* view, CInfo* info, CSettings* settings)
{
    auto total { new OrderTotalR(settings->value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kAmount), total);
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSettled), total);

    auto second { new TreeDoubleSpinR(settings->ratio_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSecond), second);

    auto first { new TreeSpinR(view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFirst), first);

    auto discount { new TreeDoubleSpinR(settings->value_decimal, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDiscount), discount);

    auto employee { new SpecificUnit(stakeholder_tree_.model, UNIT_EMPLOYEE, view) };
    view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kEmployee), employee);

    auto unit_party { info->section == Section::kSales ? UNIT_CUSTOMER : UNIT_VENDOR };
    auto party { new SpecificUnit(stakeholder_tree_.model, unit_party, view) };
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
    connect(table_sql, &Sqlite::SUpdateMultiLeafTotal, model, &TreeModel::RUpdateMultiLeafTotal);

    connect(table_sql, &Sqlite::SFreeView, this, &MainWindow::RFreeView);
}

void MainWindow::InsertNode(TreeWidget* tree_widget)
{
    auto current_index { tree_widget->View()->currentIndex() };
    current_index = current_index.isValid() ? current_index : QModelIndex();

    auto parent_index { current_index.parent() };
    parent_index = parent_index.isValid() ? parent_index : QModelIndex();

    const int parent_id { parent_index.isValid() ? parent_index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() : -1 };
    InsertNodeFunction(parent_index, parent_id, current_index.row() + 1);
}

void MainWindow::InsertNodeFunction(const QModelIndex& parent, int parent_id, int row)
{
    auto model { tree_->model };

    auto node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = model->Rule(parent_id);
    node->unit = model->Unit(parent_id);
    model->SetParent(node, parent_id);

    auto section { data_->info.section };

    if (section == Section::kSales || section == Section::kPurchase)
        InsertNodePS(section, model, node, parent, row);

    if (section != Section::kSales && section != Section::kPurchase)
        InsertNodeFPST(section, model, node, parent, parent_id, row);
}

void MainWindow::RRemoveTriggered()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget)
        return;

    if (IsTreeWidget(current_widget))
        RemoveNode(tree_->widget->View(), tree_->model);

    if (IsTableWidget(current_widget))
        RemoveTrans(current_widget);
}

void MainWindow::RemoveNode(QTreeView* view, TreeModel* model)
{
    if (!HasSelection(view))
        return;

    auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() };
    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kBranch)).data().toBool() };

    if (branch) {
        if (model->ChildrenEmpty(node_id))
            model->RemoveNode(index.row(), index.parent());
        else
            RemoveBranch(model, index, node_id);

        return;
    }

    auto& sql { data_->sql };
    bool interal_reference { sql->InternalReference(node_id) };
    bool exteral_reference { sql->ExternalReference(node_id) };

    if (!interal_reference && !exteral_reference) {
        RemoveView(model, index, node_id);
        return;
    }

    const int unit { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kUnit)).data().toInt() };

    auto dialog { new class RemoveNode(model, node_id, unit, exteral_reference, this) };
    connect(dialog, &RemoveNode::SRemoveNode, sql.data(), &Sqlite::RRemoveNode);
    connect(dialog, &RemoveNode::SReplaceNode, sql.data(), &Sqlite::RReplaceNode);
    dialog->exec();
}

void MainWindow::RemoveTrans(QWidget* widget)
{
    // 1. 获取视图和检查选择
    auto* view { GetQTableView(widget) };
    if (!view || !HasSelection(view)) {
        return;
    }

    // 2. 获取模型和当前索引
    auto* model { GetTableModel(view) };
    QModelIndex current_index { view->currentIndex() };
    if (!model || !current_index.isValid()) {
        return;
    }

    // 3. 删除行
    const int current_row { current_index.row() };
    if (!model->removeRows(current_row, 1)) {
        qDebug() << "Failed to remove row:" << current_row;
        return;
    }

    // 4. 更新选择
    const int new_row_count { model->rowCount() };
    if (new_row_count == 0) {
        return;
    }

    // 5. 计算新的选择位置
    QModelIndex new_index;
    if (current_row <= new_row_count - 1) {
        // 如果删除的不是最后一行，选择同一位置的下一条记录
        new_index = model->index(current_row, 0);
    } else {
        // 如果删除的是最后一行，选择上一条记录
        new_index = model->index(new_row_count - 1, 0);
    }

    // 6. 更新视图的当前索引
    if (new_index.isValid())
        view->setCurrentIndex(new_index);
}

void MainWindow::RemoveView(TreeModel* model, const QModelIndex& index, int node_id)
{
    model->RemoveNode(index.row(), index.parent());
    auto widget { table_hash_->value(node_id) };

    if (widget) {
        FreeWidget(widget);
        table_hash_->remove(node_id);
        SignalStation::Instance().DeregisterModel(data_->info.section, node_id);
    }
}

void MainWindow::SaveTableWidget(const TableHash& table_hash, CString& section_name, CString& property)
{
    auto keys { table_hash.keys() };
    QStringList list {};

    for (int node_id : keys)
        list.emplaceBack(QString::number(node_id));

    exclusive_interface_->setValue(QString("%1/%2").arg(section_name, property), list);
}

void MainWindow::RestoreTableWidget(Data* data, TreeModel* tree_model, CSettings* settings, TableHash* table_hash, CString& property)
{
    auto variant { exclusive_interface_->value(QString("%1/%2").arg(data->info.node, property)) };

    QSet<int> list {};

    if (variant.isValid() && variant.canConvert<QStringList>()) {
        auto variant_list { variant.value<QStringList>() };
        for (CString& node_id : variant_list)
            list.insert(node_id.toInt());
    }

    for (int node_id : list) {
        if (tree_model->Contains(node_id) && !tree_model->Branch(node_id) && node_id >= 1)
            CreateTable(data, tree_model, settings, table_hash, node_id);
    }
}

void MainWindow::Recent()
{
    recent_list_ = shared_interface_->value(RECENT_FILE).toStringList();

    auto recent_menu { ui->menuRecent };
    QStringList valid_list {};

    for (CString& file : recent_list_) {
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
    auto widget { table_hash_->value(node_id) };

    FreeWidget(widget);
    table_hash_->remove(node_id);

    SignalStation::Instance().DeregisterModel(data_->info.section, node_id);
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
    view->setColumnHidden(std::to_underlying(TableEnum::kID), false);

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
    connect(ui->actionAppendNode, &QAction::triggered, this, &MainWindow::RAppendNodeTriggered);
    connect(ui->actionJump, &QAction::triggered, this, &MainWindow::RJumpTriggered);
    connect(ui->actionSearch, &QAction::triggered, this, &MainWindow::RSearchTriggered);
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::RPreferencesTriggered);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::RAboutTriggered);
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::RNewTriggered);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::ROpenTriggered);
    connect(ui->actionClearMenu, &QAction::triggered, this, &MainWindow::RClearMenuTriggered);

    connect(ui->actionDocument, &QAction::triggered, this, &MainWindow::REditDocument);
    connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::REditNode);

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

    sql_.QuerySettings(finance_settings_, section);

    sql = QSharedPointer<SqliteFinance>::create(info);

    model = new TreeModelFinanceTask(sql, info, finance_settings_.base_unit, finance_table_hash_, interface_.separator, this);
    finance_tree_.widget = new TreeWidgetCommon(model, info, finance_settings_, this);
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

    QStringList unit_list { {}, tr("POSITION"), tr("BOX"), tr("PIECE"), tr("SET"), tr("SQUARE_FEET") };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySettings(product_settings_, section);

    sql = QSharedPointer<SqliteProduct>::create(info);

    model = new TreeModelProduct(sql, info, product_settings_.base_unit, product_table_hash_, interface_.separator, this);
    product_tree_.widget = new TreeWidgetCommon(model, info, product_settings_, this);
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

    QStringList unit_list { tr("EMPLOYEE"), tr("CUSTOMER"), tr("VENDOR"), tr("PRODUCT") };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySettings(stakeholder_settings_, section);

    sql = QSharedPointer<SqliteStakeholder>::create(info);

    stakeholder_tree_.model = new TreeModelStakeholder(sql, info, stakeholder_settings_.base_unit, stakeholder_table_hash_, interface_.separator, this);
    stakeholder_tree_.widget = new TreeWidgetStakeholder(model, info, stakeholder_settings_, this);

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

    QStringList unit_list { {}, tr("POSITION"), tr("BOX"), tr("PIECE"), tr("SET"), tr("SQUARE_FEET") };
    auto& unit_hash { info.unit_hash };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_hash.insert(i, unit_list.at(i));

    sql_.QuerySettings(task_settings_, section);

    sql = QSharedPointer<SqliteTask>::create(info);

    model = new TreeModelFinanceTask(sql, info, task_settings_.base_unit, task_table_hash_, interface_.separator, this);
    task_tree_.widget = new TreeWidgetCommon(model, info, task_settings_, this);
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

    sql_.QuerySettings(sales_settings_, section);

    sql = QSharedPointer<SqliteSales>::create(info);

    model = new TreeModelOrder(sql, info, sales_settings_.base_unit, sales_table_hash_, interface_.separator, this);
    sales_tree_.widget = new TreeWidgetOrder(model, info, sales_settings_, this);

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

    sql_.QuerySettings(purchase_settings_, section);

    sql = QSharedPointer<SqlitePurchase>::create(info);

    model = new TreeModelOrder(sql, info, purchase_settings_.base_unit, purchase_table_hash_, interface_.separator, this);
    purchase_tree_.widget = new TreeWidgetOrder(model, info, purchase_settings_, this);

    connect(product_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
    connect(stakeholder_data_.sql.data(), &Sqlite::SUpdateProductReference, sql.data(), &Sqlite::RUpdateProductReference);
}

void MainWindow::SetDateFormat() { date_format_list_.emplaceBack(DATE_TIME_FST); }

void MainWindow::SetHeader()
{
    finance_data_.info.tree_header
        = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Foreign Total"), tr("Local Total"), "" };
    finance_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("FXRate"), tr("Description"), tr("D"), tr("S"), tr("RelatedNode"), tr("Debit"),
        tr("Credit"), tr("Subtotal") };
    finance_data_.info.search_trans_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("LhsNode"), tr("LhsFXRate"), tr("LhsDebit"), tr("LhsCredit"),
        tr("Description"), {}, {}, {}, {}, tr("D"), tr("S"), tr("RhsCredit"), tr("RhsDebit"), tr("RhsFXRate"), tr("RhsNode") };
    finance_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {}, {}, {},
        {}, {}, {}, {}, tr("Foreign Total"), tr("Base Total") };

    product_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Color"),
        tr("UnitPrice"), tr("Commission"), tr("Quantity Total"), tr("Amount Total"), "" };
    product_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("UnitCost"), tr("Description"), tr("D"), tr("S"), tr("RelatedNode"),
        tr("Debit"), tr("Credit"), tr("Subtotal") };
    product_data_.info.search_trans_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("LhsNode"), {}, tr("LhsDebit"), tr("LhsCredit"), tr("Description"),
        tr("UnitCost"), {}, {}, {}, tr("D"), tr("S"), tr("RhsCredit"), tr("RhsDebit"), {}, tr("RhsNode") };
    product_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {}, {},
        tr("Color"), tr("UnitPrice"), tr("Commission"), {}, {}, tr("Quantity Total"), tr("Amount Total") };

    stakeholder_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Term"), tr("Branch"), tr("Mark"),
        tr("Deadline"), tr("Employee"), tr("PaymentPeriod"), tr("TaxRate"), "" };
    stakeholder_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("UnitPrice"), tr("Description"), tr("D"), tr("S"), tr("Inside") };
    stakeholder_data_.info.search_trans_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("Node"), {}, {}, {}, tr("Description"), tr("UnitPrice"), {}, {}, {},
        tr("D"), tr("S"), {}, {}, {}, tr("InsideProduct") };
    stakeholder_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Term"), tr("Branch"), tr("Mark"),
        tr("Deadline"), tr("Employee"), {}, tr("PaymentPeriod"), tr("TaxRate"), {}, {}, {}, {} };

    task_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Quantity Total"),
        tr("Amount Total"), "" };
    task_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("UnitCost"), tr("Description"), tr("D"), tr("S"), tr("RelatedNode"), tr("Debit"),
        tr("Credit"), tr("Subtotal") };
    task_data_.info.search_trans_header = product_data_.info.search_trans_header;
    task_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {}, {},
        tr("Color"), {}, {}, {}, {}, tr("Quantity Total"), tr("Amount Total") };

    sales_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Party"),
        tr("Employee"), tr("DateTime"), tr("First"), tr("Second"), tr("Discount"), tr("Locked"), tr("Initial Total"), tr("Final Total") };
    sales_data_.info.table_header = { tr("ID"), tr("InsideProduct"), tr("UnitPrice"), tr("Code"), tr("Description"), tr("Color"), tr("node_id"), tr("First"),
        tr("Second"), tr("Subamount"), tr("DiscountPrice"), tr("Subdiscount"), tr("Subsettled"), tr("OutsideProduct") };
    sales_data_.info.search_trans_header = { tr("ID"), {}, tr("Code"), tr("InsideProduct"), {}, tr("First"), tr("Second"), tr("Description"), tr("UnitPrice"),
        tr("NodeID"), tr("DiscountPrice"), tr("Subsettled"), {}, {}, tr("InitialSubtotal"), tr("discount"), {}, tr("OutsideProduct") };
    sales_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Party"),
        tr("Employee"), tr("DateTime"), tr("First"), tr("Second"), tr("Discount"), tr("Locked"), tr("Initial Total"), tr("Final Total") };

    purchase_data_.info.tree_header = sales_data_.info.tree_header;
    purchase_data_.info.table_header = sales_data_.info.table_header;
    purchase_data_.info.search_trans_header = sales_data_.info.search_trans_header;
    purchase_data_.info.search_node_header = sales_data_.info.search_node_header;
}

void MainWindow::SetAction()
{
    ui->actionInsert->setIcon(QIcon(":/solarized_dark/solarized_dark/insert.png"));
    ui->actionEdit->setIcon(QIcon(":/solarized_dark/solarized_dark/edit.png"));
    ui->actionDocument->setIcon(QIcon(":/solarized_dark/solarized_dark/edit2.png"));
    ui->actionRemove->setIcon(QIcon(":/solarized_dark/solarized_dark/remove2.png"));
    ui->actionAbout->setIcon(QIcon(":/solarized_dark/solarized_dark/about.png"));
    ui->actionAppendNode->setIcon(QIcon(":/solarized_dark/solarized_dark/append.png"));
    ui->actionJump->setIcon(QIcon(":/solarized_dark/solarized_dark/jump.png"));
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
    header->setSectionResizeMode(std::to_underlying(TreeEnumCommon::kDescription), QHeaderView::Stretch);
    header->setStretchLastSection(true);
    header->setDefaultAlignment(Qt::AlignCenter);
}

void MainWindow::RAppendNodeTriggered()
{
    auto current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTreeWidget(current_widget))
        return;

    auto view { tree_->widget->View() };
    if (!HasSelection(view))
        return;

    auto parent_index { view->currentIndex() };
    if (!parent_index.isValid())
        return;

    const bool branch { parent_index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kBranch)).data().toBool() };
    if (!branch)
        return;

    const int parent_id { parent_index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() };
    InsertNodeFunction(parent_index, parent_id, 0);
}

void MainWindow::AppendTrans(TableWidget* table_widget)
{
    if (!table_widget)
        return;

    auto view { GetQTableView(table_widget) };
    auto model { GetTableModel(view) };
    if (!model)
        return;

    constexpr int ID_ZERO = 0;
    const int empty_row = model->GetRhsNodeRow(ID_ZERO);

    QModelIndex target_index {};

    if (empty_row == -1) {
        const int new_row = model->rowCount();
        if (!model->insertRows(new_row, 1))
            return;

        target_index = model->index(new_row, std::to_underlying(TableEnum::kDateTime));
    } else {
        target_index = model->index(empty_row, std::to_underlying(TableEnum::kRhsNode));
    }

    view->setCurrentIndex(target_index);
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
    const int rhs_node_id { index.sibling(row, std::to_underlying(TableEnum::kRhsNode)).data().toInt() };
    if (rhs_node_id == 0)
        return;

    if (!table_hash_->contains(rhs_node_id))
        CreateTable(data_, tree_->model, settings_, table_hash_, rhs_node_id);

    const int trans_id { index.sibling(row, std::to_underlying(TableEnum::kID)).data().toInt() };
    SwitchTab(rhs_node_id, trans_id);
}

void MainWindow::RTreeViewCustomContextMenuRequested(const QPoint& pos)
{
    Q_UNUSED(pos);

    auto menu = new QMenu(this);
    menu->addAction(ui->actionInsert);
    menu->addAction(ui->actionEdit);
    menu->addAction(ui->actionAppendNode);
    menu->addAction(ui->actionRemove);

    menu->exec(QCursor::pos());
}

void MainWindow::REditNode()
{
    const auto widget { ui->tabWidget->currentWidget() };
    if (!widget || !IsTreeWidget(widget))
        return;

    const auto view { tree_->widget->View() };
    if (!HasSelection(view))
        return;

    const auto& index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { tree_->model };
    const auto& info { data_->info };
    auto section { info.section };
    CStringHash& unit_hash { info.unit_hash };

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() };

    if (section == Section::kSales || section == Section::kPurchase) {
        auto node_shadow { ResourcePool<NodeShadow>::Instance().Allocate() };
        model->SetNodeShadow(node_shadow, node_id);
        EditNodePS(section, node_shadow);
    }

    if (section != Section::kSales && section != Section::kPurchase) {
        auto tmp_node { ResourcePool<Node>::Instance().Allocate() };
        model->CopyNode(tmp_node, node_id);
        EditNodeFPST(section, model, tmp_node, index, node_id, unit_hash);
    }
}

void MainWindow::EditNodePS(Section section, NodeShadow* node_shadow)
{
    QDialog* dialog {};
    auto sql { data_->sql };

    auto table_model { new TableModelOrder(sql, *node_shadow->rule, *node_shadow->id, data_->info, product_tree_.model, this) };

    switch (section) {
    case Section::kSales:
        dialog = new EditNodeOrder(node_shadow, sql, table_model, stakeholder_tree_.model, sales_settings_.value_decimal, UNIT_CUSTOMER, this);
        dialog->setWindowTitle(tr(Sales));
        break;
    case Section::kPurchase:
        dialog = new EditNodeOrder(node_shadow, sql, table_model, stakeholder_tree_.model, sales_settings_.value_decimal, UNIT_VENDOR, this);
        dialog->setWindowTitle(tr(Purchase));
        break;
    default:
        return ResourcePool<NodeShadow>::Instance().Recycle(node_shadow);
    }

    connect(dialog, &QDialog::rejected, this, [=, this]() {
        dialog_list_->removeOne(dialog);
        ResourcePool<NodeShadow>::Instance().Recycle(node_shadow);
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);

    assert(dynamic_cast<EditNodeOrder*>(dialog) && "Dialog is not EditNodeOrder");
    assert(dynamic_cast<TreeModelOrder*>(tree_->model) && "Model is not TreeModelOrder");

    auto dialog_cast { static_cast<EditNodeOrder*>(dialog) };
    auto tree_model { static_cast<TreeModelOrder*>(tree_->model) };

    connect(stakeholder_tree_.model, &TreeModelStakeholder::SUpdateOrderPartyEmployee, dialog_cast, &EditNodeOrder::RUpdateStakeholder);
    connect(tree_model, &TreeModelOrder::SUpdateLocked, dialog_cast, &EditNodeOrder::RUpdateLocked);
    connect(dialog_cast, &EditNodeOrder::SUpdateLocked, tree_model, &TreeModelOrder::RUpdateLocked);
    connect(table_model, &TableModelOrder::SUpdateFirst, tree_model, &TreeModelOrder::RUpdateFirst);

    if (!(*node_shadow->branch)) {
        SetView(dialog_cast->View());
        DelegateOrder(dialog_cast->View(), settings_);
    }
    dialog_list_->append(dialog);
    dialog->show();
}

void MainWindow::EditNodeFPST(Section section, TreeModel* model, Node* tmp_node, const QModelIndex& index, int node_id, CStringHash& unit_hash)
{
    QDialog* dialog {};

    const auto& parent { index.parent() };
    const int parent_id { parent.isValid() ? parent.siblingAtColumn(std::to_underlying(TreeEnumCommon::kID)).data().toInt() : -1 };
    auto parent_path { model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& name_list { model->ChildrenName(parent_id, node_id) };
    const auto& sqlite { data_->sql };

    bool enable_branch { !sqlite->InternalReference(node_id) && !sqlite->ExternalReference(node_id) && !table_hash_->contains(node_id)
        && model->ChildrenEmpty(node_id) };

    switch (section) {
    case Section::kFinance:
        dialog = new EditNodeFinance(tmp_node, unit_hash, parent_path, name_list, enable_branch, this);
        break;
    case Section::kTask:
        dialog = new EditNodeFinance(tmp_node, unit_hash, parent_path, name_list, enable_branch, this);
        break;
    case Section::kStakeholder:
        dialog = new EditNodeStakeholder(tmp_node, unit_hash, parent_path, name_list, enable_branch, settings_->ratio_decimal, model, this);
        break;
    case Section::kProduct:
        dialog = new EditNodeProduct(tmp_node, unit_hash, parent_path, name_list, enable_branch, settings_->ratio_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(tmp_node);
    }

    connect(dialog, &QDialog::accepted, this, [=]() { model->UpdateNode(tmp_node); });
    dialog->exec();
    ResourcePool<Node>::Instance().Recycle(tmp_node);
}

void MainWindow::InsertNodeFPST(Section section, TreeModel* model, Node* node, const QModelIndex& parent, int parent_id, int row)
{
    auto parent_path { model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& info { data_->info };
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
        dialog = new EditNodeStakeholder(node, info.unit_hash, parent_path, name_list, true, settings_->ratio_decimal, model, this);
        break;
    case Section::kProduct:
        dialog = new EditNodeProduct(node, info.unit_hash, parent_path, name_list, true, settings_->ratio_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(node);
    }

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (model->InsertNode(row, parent, node)) {
            auto index = model->index(row, 0, parent);
            tree_->widget->SetCurrentIndex(index);
        }
    });

    connect(dialog, &QDialog::rejected, this, [=]() { ResourcePool<Node>::Instance().Recycle(node); });
    dialog->exec();
}

void MainWindow::InsertNodePS(Section section, TreeModel* model, Node* node, const QModelIndex& parent, int row)
{
    QDialog* dialog {};
    auto sql { data_->sql };

    auto table_model { new TableModelOrder(sql, node->rule, 0, data_->info, product_tree_.model, this) };

    switch (section) {
    case Section::kSales:
        dialog = new InsertNodeOrder(node, sql, table_model, stakeholder_tree_.model, purchase_settings_.value_decimal, UNIT_CUSTOMER, this);
        dialog->setWindowTitle(tr(Sales));
        break;
    case Section::kPurchase:
        dialog = new InsertNodeOrder(node, sql, table_model, stakeholder_tree_.model, purchase_settings_.value_decimal, UNIT_VENDOR, this);
        dialog->setWindowTitle(tr(Purchase));
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(node);
    }

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (model->InsertNode(row, parent, node)) {
            auto index = model->index(row, 0, parent);
            tree_->widget->SetCurrentIndex(index);
        }
    });
    connect(dialog, &QDialog::rejected, this, [=, this]() { dialog_list_->removeOne(dialog); });

    dialog->setAttribute(Qt::WA_DeleteOnClose);

    assert(dynamic_cast<InsertNodeOrder*>(dialog) && "Dialog is not InsertNodeOrder");
    assert(dynamic_cast<TreeModelOrder*>(tree_->model) && "Model is not TreeModelOrder");

    auto dialog_cast { static_cast<InsertNodeOrder*>(dialog) };
    auto tree_model { static_cast<TreeModelOrder*>(model) };

    connect(stakeholder_tree_.model, &TreeModelStakeholder::SUpdateOrderPartyEmployee, dialog_cast, &InsertNodeOrder::RUpdateStakeholder);
    connect(tree_model, &TreeModelOrder::SUpdateLocked, dialog_cast, &InsertNodeOrder::RUpdateLocked);
    connect(dialog_cast, &InsertNodeOrder::SUpdateLocked, tree_model, &TreeModelOrder::RUpdateLocked);
    connect(table_model, &TableModelOrder::SUpdateFirst, tree_model, &TreeModelOrder::RUpdateFirst);

    dialog_list_->append(dialog);

    SetView(dialog_cast->View());
    DelegateOrder(dialog_cast->View(), settings_);
    dialog->show();
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

    auto document_dir { settings_->document_dir };
    auto model { GetTableModel(view) };
    auto document_pointer { model->GetDocumentPointer(trans_index) };
    const int trans_id { trans_index.siblingAtColumn(std::to_underlying(TableEnum::kID)).data().toInt() };

    auto dialog { new EditDocument(data_->info.section, document_pointer, document_dir, this) };
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->UpdateField(data_->info.transaction, document_pointer->join(SEMICOLON), DOCUMENT, trans_id);
}

void MainWindow::RUpdateName(const Node* node)
{
    auto model { tree_->model };
    int node_id { node->id };
    auto tab_bar { ui->tabWidget->tabBar() };
    int count { ui->tabWidget->count() };

    if (!node->branch) {
        if (!table_hash_->contains(node_id))
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
                for (const auto* child : queue_node->children)
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

void MainWindow::RUpdateSettings(CSettings& settings, const Interface& interface)
{
    if (interface_ != interface)
        UpdateInterface(interface);

    if (*settings_ != settings) {
        bool update_base_unit { settings_->base_unit != settings.base_unit };
        bool resize_column { settings_->value_decimal != settings.value_decimal || settings_->ratio_decimal != settings.ratio_decimal
            || settings_->hide_time != settings.hide_time };

        *settings_ = settings;

        if (update_base_unit)
            tree_->model->UpdateBaseUnit(settings.base_unit);

        tree_->widget->SetStatus();
        sql_.UpdateSettings(settings, data_->info.section);

        if (resize_column) {
            auto current_widget { ui->tabWidget->currentWidget() };
            if (IsTableWidget(current_widget))
                ResizeColumn(GetQTableView(current_widget)->horizontalHeader(), true);
            if (IsTreeWidget(current_widget))
                ResizeColumn(tree_->widget->Header(), false);
        }
    }
}
void MainWindow::RFreeView(int node_id)
{
    auto view { table_hash_->value(node_id) };

    if (view) {
        FreeWidget(view);
        table_hash_->remove(node_id);
        SignalStation::Instance().DeregisterModel(data_->info.section, node_id);
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
               : header->setSectionResizeMode(std::to_underlying(TreeEnumCommon::kDescription), QHeaderView::Stretch);
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
    QString command { "D:/Qt/6.7.3/llvm-mingw_64/bin/rcc.exe" };
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
    QString command { QDir::homePath() + "/Qt/6.7.3/macos/libexec/rcc" + " -binary " + QDir::homePath() + "/Documents/YTX/resource/resource.qrc -o " + path };

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

    auto dialog { new Search(data_->info, tree_->model, stakeholder_tree_.model, data_->sql, data_->info.rule_hash, *settings_, this) };
    connect(dialog, &Search::STreeLocation, this, &MainWindow::RTreeLocation);
    connect(dialog, &Search::STableLocation, this, &MainWindow::RTableLocation);
    connect(tree_->model, &TreeModel::SSearch, dialog, &Search::RSearch);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog_list_->append(dialog);
    dialog->show();
}

void MainWindow::RTreeLocation(int node_id)
{
    auto widget { tree_->widget };
    ui->tabWidget->setCurrentWidget(widget);

    auto index { tree_->model->GetIndex(node_id) };
    widget->activateWindow();
    widget->SetCurrentIndex(index);
}

void MainWindow::RTableLocation(int trans_id, int lhs_node_id, int rhs_node_id)
{
    int id { lhs_node_id };

    auto Contains = [&](int node_id) {
        if (table_hash_->contains(node_id)) {
            id = node_id;
            return true;
        }
        return false;
    };

    if (!Contains(lhs_node_id) && !Contains(rhs_node_id))
        CreateTable(data_, tree_->model, settings_, table_hash_, id);

    SwitchTab(id, trans_id);
}

void MainWindow::RPreferencesTriggered()
{
    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    auto model { tree_->model };

    auto preference { new Preferences(data_->info, model, date_format_list_, interface_, *settings_, this) };
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

    SwitchDialog(dialog_list_, true);
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
    if (data_) {
        auto index { ui->tabWidget->currentIndex() };
        data_->tab = ui->tabWidget->tabBar()->tabData(index).value<Tab>();
    }
}

void MainWindow::on_rBtnFinance_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kFinance;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    tree_ = &finance_tree_;
    table_hash_ = &finance_table_hash_;
    dialog_list_ = &finance_dialog_list_;
    settings_ = &finance_settings_;
    data_ = &finance_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnSales_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kSales;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    tree_ = &sales_tree_;
    table_hash_ = &sales_table_hash_;
    dialog_list_ = &sales_dialog_list_;
    settings_ = &sales_settings_;
    data_ = &sales_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnTask_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kTask;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    tree_ = &task_tree_;
    table_hash_ = &task_table_hash_;
    dialog_list_ = &task_dialog_list_;
    settings_ = &task_settings_;
    data_ = &task_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnStakeholder_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kStakeholder;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    tree_ = &stakeholder_tree_;
    table_hash_ = &stakeholder_table_hash_;
    dialog_list_ = &stakeholder_dialog_list_;
    settings_ = &stakeholder_settings_;
    data_ = &stakeholder_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnProduct_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kProduct;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    tree_ = &product_tree_;
    table_hash_ = &product_table_hash_;
    dialog_list_ = &product_dialog_list_;
    settings_ = &product_settings_;
    data_ = &product_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnPurchase_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kPurchase;

    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    tree_ = &purchase_tree_;
    table_hash_ = &purchase_table_hash_;
    dialog_list_ = &purchase_dialog_list_;
    settings_ = &purchase_settings_;
    data_ = &purchase_data_;

    SwitchSection(data_->tab);
}
