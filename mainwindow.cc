#include "mainwindow.h"

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLockFile>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QQueue>
#include <QResource>
#include <QScrollBar>
#include <QtConcurrent>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "database/sqlite/sqlitefinance.h"
#include "database/sqlite/sqliteorder.h"
#include "database/sqlite/sqliteproduct.h"
#include "database/sqlite/sqlitestakeholder.h"
#include "database/sqlite/sqlitetask.h"
#include "delegate/checkbox.h"
#include "delegate/line.h"
#include "delegate/search/searchpathtabler.h"
#include "delegate/specificunit.h"
#include "delegate/table/colorr.h"
#include "delegate/table/helpernode.h"
#include "delegate/table/tablecombo.h"
#include "delegate/table/tabledatetime.h"
#include "delegate/table/tabledbclick.h"
#include "delegate/table/tabledoublespin.h"
#include "delegate/table/tabledoublespinr.h"
#include "delegate/tree/color.h"
#include "delegate/tree/finance/financeforeignr.h"
#include "delegate/tree/order/finished.h"
#include "delegate/tree/order/ordername.h"
#include "delegate/tree/stakeholder/deadline.h"
#include "delegate/tree/stakeholder/paymentperiod.h"
#include "delegate/tree/stakeholder/taxrate.h"
#include "delegate/tree/treecombo.h"
#include "delegate/tree/treedatetime.h"
#include "delegate/tree/treedoublespin.h"
#include "delegate/tree/treedoublespinr.h"
#include "delegate/tree/treedoublespinunitr.h"
#include "delegate/tree/treeplaintext.h"
#include "dialog/about.h"
#include "dialog/editdocument.h"
#include "dialog/editnode/editnodefinance.h"
#include "dialog/editnode/editnodeorder.h"
#include "dialog/editnode/editnodeproduct.h"
#include "dialog/editnode/editnodestakeholder.h"
#include "dialog/preferences.h"
#include "dialog/removenode.h"
#include "dialog/search.h"
#include "global/resourcepool.h"
#include "global/signalstation.h"
#include "global/sqlconnection.h"
#include "table/model/tablemodelfinance.h"
#include "table/model/tablemodelhelper.h"
#include "table/model/tablemodelproduct.h"
#include "table/model/tablemodelstakeholder.h"
#include "table/model/tablemodeltask.h"
#include "tree/model/treemodelfinance.h"
#include "tree/model/treemodelorder.h"
#include "tree/model/treemodelproduct.h"
#include "tree/model/treemodelstakeholder.h"
#include "tree/model/treemodeltask.h"
#include "ui_mainwindow.h"
#include "widget/tablewidget/tablewidgetfpts.h"
#include "widget/treewidget/treewidgetfpt.h"
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

    if (finance_tree_) {
        SaveTab(finance_table_hash_, finance_data_.info.node, VIEW);
        SaveState(finance_tree_->View()->header(), exclusive_interface_, finance_data_.info.node, HEADER_STATE);

        finance_dialog_list_.clear();
    }

    if (product_tree_) {
        SaveTab(product_table_hash_, product_data_.info.node, VIEW);
        SaveState(product_tree_->View()->header(), exclusive_interface_, product_data_.info.node, HEADER_STATE);

        product_dialog_list_.clear();
    }

    if (stakeholder_tree_) {
        SaveTab(stakeholder_table_hash_, stakeholder_data_.info.node, VIEW);
        SaveState(stakeholder_tree_->View()->header(), exclusive_interface_, stakeholder_data_.info.node, HEADER_STATE);

        stakeholder_dialog_list_.clear();
    }

    if (task_tree_) {
        SaveTab(task_table_hash_, task_data_.info.node, VIEW);
        SaveState(task_tree_->View()->header(), exclusive_interface_, task_data_.info.node, HEADER_STATE);

        task_dialog_list_.clear();
    }

    if (sales_tree_) {
        // SaveTab(sales_table_hash_, sales_data_.info.node, VIEW);
        SaveState(sales_tree_->View()->header(), exclusive_interface_, sales_data_.info.node, HEADER_STATE);

        sales_dialog_list_.clear();
    }

    if (purchase_tree_) {
        // SaveTab(purchase_table_hash_, purchase_data_.info.node, VIEW);
        SaveState(purchase_tree_->View()->header(), exclusive_interface_, purchase_data_.info.node, HEADER_STATE);

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
        const auto& complete_base_name { file_info.completeBaseName() };

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

        CreateSection(finance_tree_, finance_table_hash_, finance_data_, finance_settings_, tr(Finance));
        CreateSection(stakeholder_tree_, stakeholder_table_hash_, stakeholder_data_, stakeholder_settings_, tr(Stakeholder));
        CreateSection(product_tree_, product_table_hash_, product_data_, product_settings_, tr(Product));
        CreateSection(task_tree_, task_table_hash_, task_data_, task_settings_, tr(Task));
        CreateSection(sales_tree_, sales_table_hash_, sales_data_, sales_settings_, tr(Sales));
        CreateSection(purchase_tree_, purchase_table_hash_, purchase_data_, purchase_settings_, tr(Purchase));

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
            auto* menu { ui->menuRecent };
            auto* action { new QAction(path, menu) };

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

    if (data_->info.section == Section::kSales || data_->info.section == Section::kPurchase)
        return;

    if (IsTableWidget(widget)) {
        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        AppendTrans(static_cast<TableWidget*>(widget));
    }
}

void MainWindow::RTreeViewDoubleClicked(const QModelIndex& index)
{
    if (index.column() != 0)
        return;

    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool() };
    if (branch)
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    if (node_id <= 0)
        return;

    if (!table_hash_->contains(node_id)) {
        auto section { data_->info.section };

        if (section == Section::kSales || section == Section::kPurchase) {
            if (auto it { dialog_hash_->constFind(node_id) }; it != dialog_hash_->constEnd()) {
                auto dialog { *it };
                dialog->show();
                dialog->raise();
                dialog->activateWindow();
                return;
            }

            const int party_id { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)).data().toInt() };
            if (party_id <= 0)
                return;

            CreateTableOrder(tree_widget_->Model(), table_hash_, data_, settings_, node_id, party_id);
        }

        if (section != Section::kSales && section != Section::kPurchase) {
            const bool is_helper { index.siblingAtColumn(std::to_underlying(TreeEnum::kIsHelper)).data().toBool() };

            if (is_helper)
                CreateTableHelper(tree_widget_->Model(), table_hash_, data_, settings_, node_id);
            else
                CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, node_id);
        }
    }

    SwitchTab(node_id);
}

void MainWindow::SwitchTab(int node_id, int trans_id) const
{
    auto* widget { table_hash_->value(node_id, nullptr) };
    if (!widget)
        return;

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();

    if (trans_id == 0)
        return;

    auto view { widget->View() };
    auto index { GetTableModel(widget)->GetIndex(trans_id) };
    view->setCurrentIndex(index);
    view->scrollTo(index.siblingAtColumn(std::to_underlying(TableEnum::kDateTime)), QAbstractItemView::PositionAtCenter);
}

bool MainWindow::LockFile(CString& absolute_path, CString& complete_base_name) const
{
    auto lock_file_path { absolute_path + SLASH + complete_base_name + SFX_LOCK };

    static QLockFile lock_file { lock_file_path };
    if (!lock_file.tryLock(100))
        return false;

    return true;
}

void MainWindow::CreateTableFPTS(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id)
{
    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const auto& info { data->info };
    auto section { info.section };
    auto rule { tree_model->Rule(node_id) };

    TableModel* model {};

    switch (section) {
    case Section::kFinance:
        model = new TableModelFinance(sql, rule, node_id, info, this);
        break;
    case Section::kProduct:
        model = new TableModelProduct(sql, rule, node_id, info, this);
        break;
    case Section::kTask:
        model = new TableModelTask(sql, rule, node_id, info, this);
        break;
    case Section::kStakeholder:
        model = new TableModelStakeholder(sql, rule, node_id, info, this);
        break;
    default:
        break;
    }

    TableWidgetFPTS* widget { new TableWidgetFPTS(model, this) };

    int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetView(view);

    switch (section) {
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        TableConnectFPT(view, model, tree_model, data);
        DelegateFPTS(view);
        break;
    case Section::kStakeholder:
        TableConnectStakeholder(view, model, tree_model, data);
        break;
    default:
        break;
    }

    switch (section) {
    case Section::kFinance:
        DelegateFinance(view, tree_model, settings, node_id);
        break;
    case Section::kProduct:
        DelegateProduct(view, tree_model, settings, node_id);
        break;
    case Section::kTask:
        DelegateTask(view, tree_model, settings, node_id);
        break;
    case Section::kStakeholder:
        DelegateStakeholder(view, tree_model, settings);
        break;
    default:
        break;
    }

    table_hash->insert(node_id, widget);
    SignalStation::Instance().RegisterModel(section, node_id, model);
}

void MainWindow::CreateTableHelper(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id)
{
    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const auto& info { data->info };
    auto section { info.section };
    auto rule { tree_model->Rule(node_id) };

    auto* model { new TableModelHelper(sql, rule, node_id, info, this) };
    TableWidgetFPTS* widget { new TableWidgetFPTS(model, this) };

    int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetHelperView(view);
    DelegateHelper(view, tree_model, settings);

    table_hash->insert(node_id, widget);
    SignalStation::Instance().RegisterModel(section, node_id, model);

    connect(data->sql, &Sqlite::SRemoveMultiTrans, model, &TableModel::RRemoveMultiTrans);
}

void MainWindow::CreateTableOrder(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id, int party_id)
{
    const auto& info { data->info };
    auto section { info.section };

    if (section != Section::kSales && section != Section::kPurchase)
        return;

    auto* node_shadow { ResourcePool<NodeShadow>::Instance().Allocate() };
    tree_model->SetNodeShadowOrder(node_shadow, node_id);

    auto* sql { data->sql };

    TableModelOrder* model { new TableModelOrder(sql, true, node_id, info, node_shadow, product_tree_->Model(), stakeholder_data_.sql, this) };
    TableWidgetOrder* widget { new TableWidgetOrder(node_shadow, sql, model, stakeholder_tree_->Model(), settings, section, this) };

    int tab_index { ui->tabWidget->addTab(widget, stakeholder_tree_->Model()->Name(party_id)) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, stakeholder_tree_->Model()->GetPath(party_id));

    auto view { widget->View() };
    SetView(view);

    TableConnectOrder(view, model, tree_model, widget);
    DelegateOrder(view, settings);

    table_hash->insert(node_id, widget);
}

void MainWindow::TableConnectFPT(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, const Data* data) const
{
    connect(table_model, &TableModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TableModel::SSearch, tree_model, &TreeModel::RSearch);

    connect(table_model, &TableModel::SUpdateLeafValue, tree_model, &TreeModel::RUpdateLeafValue);
    connect(table_model, &TableModel::SUpdateLeafValueOne, tree_model, &TreeModel::RUpdateLeafValueOne);

    connect(table_model, &TableModel::SRemoveOneTrans, &SignalStation::Instance(), &SignalStation::RRemoveOneTrans);
    connect(table_model, &TableModel::SAppendOneTrans, &SignalStation::Instance(), &SignalStation::RAppendOneTrans);
    connect(table_model, &TableModel::SUpdateBalance, &SignalStation::Instance(), &SignalStation::RUpdateBalance);
    connect(table_model, &TableModel::SRemoveHelperTrans, &SignalStation::Instance(), &SignalStation::RRemoveHelperTrans);
    connect(table_model, &TableModel::SAppendHelperTrans, &SignalStation::Instance(), &SignalStation::RAppendHelperTrans);

    connect(data->sql, &Sqlite::SRemoveMultiTrans, table_model, &TableModel::RRemoveMultiTrans);
    connect(data->sql, &Sqlite::SMoveMultiTrans, table_model, &TableModel::RMoveMultiTrans);
}

void MainWindow::TableConnectOrder(PQTableView table_view, TableModelOrder* table_model, PTreeModel tree_model, TableWidgetOrder* widget) const
{
    connect(table_model, &TableModel::SSearch, tree_model, &TreeModel::RSearch);
    connect(stakeholder_tree_->Model(), &TreeModelStakeholder::SUpdateComboModel, widget, &TableWidgetOrder::RUpdateComboModel);
    connect(table_model, &TableModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);

    connect(table_model, &TableModel::SUpdateLeafValue, tree_model, &TreeModel::RUpdateLeafValue);
    connect(table_model, &TableModel::SUpdateLeafValueOne, tree_model, &TreeModel::RUpdateLeafValueOne);

    connect(table_model, &TableModel::SUpdateLeafValue, widget, &TableWidgetOrder::RUpdateLeafValue);
    connect(table_model, &TableModel::SUpdateLeafValueOne, widget, &TableWidgetOrder::RUpdateLeafValueOne);

    assert(dynamic_cast<TreeModelOrder*>(tree_model.data()) && "Tree Model is not TreeModelOrder");
    auto* tree_model_order { static_cast<TreeModelOrder*>(tree_model.data()) };

    connect(tree_model_order, &TreeModelOrder::SUpdateData, widget, &TableWidgetOrder::RUpdateData);
    connect(widget, &TableWidgetOrder::SUpdateFinished, tree_model_order, &TreeModelOrder::RUpdateFinished);
    connect(widget, &TableWidgetOrder::SUpdateFinished, table_model, &TableModelOrder::RUpdateFinished);
    connect(widget, &TableWidgetOrder::SUpdateParty, this, &MainWindow::RUpdateParty);
    connect(widget, &TableWidgetOrder::SUpdateParty, table_model, &TableModelOrder::RUpdateParty);
}

void MainWindow::TableConnectStakeholder(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, const Data* data) const
{
    connect(table_model, &TableModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TableModel::SSearch, tree_model, &TreeModel::RSearch);

    connect(data->sql, &Sqlite::SMoveMultiTrans, table_model, &TableModel::RMoveMultiTrans);
    connect(data->sql, &Sqlite::SRemoveMultiTrans, table_model, &TableModel::RRemoveMultiTrans);

    connect(table_model, &TableModel::SRemoveHelperTrans, &SignalStation::Instance(), &SignalStation::RRemoveHelperTrans);
    connect(table_model, &TableModel::SAppendHelperTrans, &SignalStation::Instance(), &SignalStation::RAppendHelperTrans);
}

void MainWindow::DelegateFPTS(PQTableView table_view) const
{
    auto* date_time { new TableDateTime(interface_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDateTime), date_time);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kCode), line);
}

void MainWindow::DelegateFinance(PQTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const
{
    auto* node { new TableCombo(tree_model, node_id, Filter::kExcludeSpecific, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kRhsNode), node);
    connect(tree_model, &TreeModel::SUpdateComboModel, node, &TableCombo::RUpdateComboModel);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kState), state);

    auto* document { new TableDbClick(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);

    auto* amount { new TableDoubleSpin(settings->amount_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kDebit), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kCredit), amount);

    auto* fx_rate { new TableDoubleSpin(settings->common_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kLhsRatio), fx_rate);

    auto* subtotal { new TableDoubleSpinR(settings->amount_decimal, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kSubtotal), subtotal);

    auto* helper_node { new HelperNode(tree_model, Filter::kIncludeAllWithNone, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kHelperNode), helper_node);
    connect(tree_model, &TreeModel::SUpdateComboModel, helper_node, &HelperNode::RUpdateComboModel);
}

void MainWindow::DelegateTask(PQTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const
{
    auto* node { new TableCombo(tree_model, node_id, Filter::kExcludeSpecific, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kRhsNode), node);
    connect(tree_model, &TreeModel::SUpdateComboModel, node, &TableCombo::RUpdateComboModel);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kState), state);

    auto* document { new TableDbClick(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);

    auto* quantity { new TableDoubleSpin(settings->common_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kDebit), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kCredit), quantity);

    auto* unit_cost { new TableDoubleSpin(settings->amount_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kUnitCost), unit_cost);

    auto* subtotal { new TableDoubleSpinR(settings->common_decimal, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kSubtotal), subtotal);

    auto* helper_node { new HelperNode(tree_model, Filter::kIncludeAllWithNone, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumTask::kHelperNode), helper_node);
    connect(tree_model, &TreeModel::SUpdateComboModel, helper_node, &HelperNode::RUpdateComboModel);
}

void MainWindow::DelegateProduct(PQTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const
{
    auto* helper_node { new HelperNode(tree_model, Filter::kIncludeAllWithNone, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kHelperNode), helper_node);
    connect(tree_model, &TreeModel::SUpdateComboModel, helper_node, &HelperNode::RUpdateComboModel);

    auto* node { new TableCombo(tree_model, node_id, Filter::kExcludeSpecific, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kRhsNode), node);
    connect(tree_model, &TreeModel::SUpdateComboModel, node, &TableCombo::RUpdateComboModel);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kState), state);

    auto* document { new TableDbClick(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);

    auto* quantity { new TableDoubleSpin(settings->common_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kDebit), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kCredit), quantity);

    auto* unit_cost { new TableDoubleSpin(settings->amount_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kUnitCost), unit_cost);

    auto* subtotal { new TableDoubleSpinR(settings->common_decimal, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumProduct::kSubtotal), subtotal);
}

void MainWindow::DelegateStakeholder(PQTableView table_view, PTreeModel tree_model, CSettings* settings) const
{
    auto product_tree_model { product_tree_->Model() };
    auto* inside_product { new SpecificUnit(product_tree_model, UNIT_POS, false, Filter::kExcludeSpecific, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kInsideProduct), inside_product);
    connect(product_tree_model, &TreeModel::SUpdateComboModel, inside_product, &SpecificUnit::RUpdateComboModel);

    auto* unit_price { new TableDoubleSpin(settings->amount_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kUnitPrice), unit_price);

    auto* date_time { new TableDateTime(interface_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kDateTime), date_time);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kCode), line);

    auto* document { new TableDbClick(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kState), state);

    auto* helper_node { new HelperNode(tree_model, Filter::kIncludeAllWithNone, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kOutsideProduct), helper_node);
    connect(tree_model, &TreeModel::SUpdateComboModel, helper_node, &HelperNode::RUpdateComboModel);
}

void MainWindow::DelegateOrder(PQTableView table_view, CSettings* settings) const
{
    auto product_tree_model { product_tree_->Model() };
    auto* inside_product { new SpecificUnit(product_tree_model, UNIT_POS, false, Filter::kExcludeSpecific, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kInsideProduct), inside_product);
    connect(product_tree_model, &TreeModel::SUpdateComboModel, inside_product, &SpecificUnit::RUpdateComboModel);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };
    auto* helper_node { new HelperNode(stakeholder_tree_model, Filter::kIncludeAllWithNone, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kOutsideProduct), helper_node);
    connect(stakeholder_tree_model, &TreeModel::SUpdateComboModel, helper_node, &HelperNode::RUpdateComboModel);

    auto* color { new ColorR(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kColor), color);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kCode), line);

    auto* price { new TableDoubleSpin(settings->amount_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kDiscountPrice), price);

    auto* quantity { new TableDoubleSpin(settings->common_decimal, DMIN, DMAX, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kSecond), quantity);

    auto* amount { new TableDoubleSpinR(settings->amount_decimal, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kDiscount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kSettled), amount);
}

void MainWindow::CreateSection(TreeWidget* tree_widget, TableHash& table_hash, CData& data, CSettings& settings, CString& name)
{
    const auto& info { data.info };
    auto* tab_widget { ui->tabWidget };

    auto view { tree_widget->View() };
    auto model { tree_widget->Model() };

    SetDelegate(view, info, settings);
    TreeConnect(tree_widget, data.sql);

    tab_widget->tabBar()->setTabData(tab_widget->addTab(tree_widget, name), QVariant::fromValue(Tab { info.section, 0 }));

    RestoreState(view->header(), exclusive_interface_, info.node, HEADER_STATE);
    RestoreTab(model, table_hash, data, settings, VIEW);

    SetView(view);
    // view->setColumnHidden(1, true);
}

void MainWindow::SetDelegate(PQTreeView tree_view, CInfo& info, CSettings& settings) const
{
    DelegateCommon(tree_view, info);

    switch (info.section) {
    case Section::kFinance:
        DelegateFinance(tree_view, info, settings);
        break;
    case Section::kTask:
        DelegateTask(tree_view, settings);
        break;
    case Section::kStakeholder:
        DelegateStakeholder(tree_view, settings);
        break;
    case Section::kProduct:
        DelegateProduct(tree_view, settings);
        break;
    case Section::kSales:
    case Section::kPurchase:
        DelegateOrder(tree_view, info, settings);
        break;
    default:
        break;
    }
}

void MainWindow::DelegateCommon(PQTreeView tree_view, CInfo& info) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kDescription), line);

    auto* plain_text { new TreePlainText(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kNote), plain_text);

    auto* rule { new TreeCombo(info.rule_map, false, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kRule), rule);

    auto* branch { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kBranch), branch);

    auto* unit { new TreeCombo(info.unit_map, false, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kUnit), unit);
}

void MainWindow::DelegateFinance(PQTreeView tree_view, CInfo& info, CSettings& settings) const
{
    auto* final_total { new TreeDoubleSpinUnitR(settings.amount_decimal, false, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinance::kFinalTotal), final_total);

    auto* initial_total { new FinanceForeignR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinance::kInitialTotal), initial_total);

    auto* helper { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinance::kIsHelper), helper);
}

void MainWindow::DelegateTask(PQTreeView tree_view, CSettings& settings) const
{
    auto* quantity { new TreeDoubleSpinR(settings.common_decimal, false, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kQuantity), quantity);

    auto* amount { new TreeDoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kAmount), amount);

    auto* unit_cost { new TreeDoubleSpin(settings.amount_decimal, DMIN, DMAX, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kUnitCost), unit_cost);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kColor), color);

    auto* date_time { new TreeDateTime(interface_.date_format, true, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kDateTime), date_time);

    auto* finished { new Finished(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kFinished), finished);

    auto* helper { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kIsHelper), helper);
}

void MainWindow::DelegateProduct(PQTreeView tree_view, CSettings& settings) const
{
    auto* quantity { new TreeDoubleSpinR(settings.common_decimal, false, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kQuantity), quantity);

    auto* amount { new TreeDoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kAmount), amount);

    auto* price { new TreeDoubleSpin(settings.amount_decimal, DMIN, DMAX, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kUnitPrice), price);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kCommission), price);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kColor), color);

    auto* helper { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kIsHelper), helper);
}

void MainWindow::DelegateStakeholder(PQTreeView tree_view, CSettings& settings) const
{
    auto* payment_period { new PaymentPeriod(0, IMAX, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kPaymentPeriod), payment_period);

    auto* tax_rate { new TaxRate(settings.amount_decimal, 0.0, DMAX, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kTaxRate), tax_rate);

    auto* deadline { new DeadLine(DD, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kDeadline), deadline);

    auto* employee { new SpecificUnit(stakeholder_tree_->Model(), UNIT_EMP, true, Filter::kIncludeSpecificWithNone, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kEmployee), employee);
    connect(stakeholder_tree_->Model(), &TreeModel::SUpdateComboModel, employee, &SpecificUnit::RUpdateComboModel);

    auto* helper { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kIsHelper), helper);
}

void MainWindow::DelegateOrder(PQTreeView tree_view, CInfo& info, CSettings& settings) const
{
    auto* rule { new TreeCombo(info.rule_map, true, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kRule), rule);

    auto* amount { new TreeDoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kAmount), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSettled), amount);

    auto* discount { new TreeDoubleSpinUnitR(settings.amount_decimal, true, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDiscount), discount);

    auto* quantity { new TreeDoubleSpinR(settings.common_decimal, true, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSecond), quantity);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFirst), quantity);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };

    auto* employee { new SpecificUnit(stakeholder_tree_model, UNIT_EMP, true, Filter::kIncludeSpecificWithNone, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kEmployee), employee);
    connect(stakeholder_tree_model, &TreeModel::SUpdateComboModel, employee, &SpecificUnit::RUpdateComboModel);

    auto party_unit { info.section == Section::kSales ? UNIT_CUST : UNIT_VEND };
    auto* name { new OrderName(stakeholder_tree_model, party_unit, Filter::kIncludeSpecific, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kName), name);
    connect(stakeholder_tree_model, &TreeModel::SUpdateComboModel, name, &OrderName::RUpdateComboModel);

    auto* date_time { new TreeDateTime(interface_.date_format, true, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDateTime), date_time);

    auto* finished { new Finished(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFinished), finished);
}

void MainWindow::TreeConnect(TreeWidget* tree_widget, const Sqlite* sql) const
{
    auto view { tree_widget->View() };
    auto model { tree_widget->Model() };

    connect(view, &QTreeView::doubleClicked, this, &MainWindow::RTreeViewDoubleClicked);
    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::RTreeViewCustomContextMenuRequested);

    connect(model, &TreeModel::SUpdateName, this, &MainWindow::RUpdateName);

    connect(model, &TreeModel::SUpdateDSpinBox, tree_widget, &TreeWidget::RUpdateDSpinBox);

    connect(model, &TreeModel::SResizeColumnToContents, view, &QTreeView::resizeColumnToContents);

    connect(model, &TreeModel::SRule, &SignalStation::Instance(), &SignalStation::RRule);

    connect(sql, &Sqlite::SRemoveNode, model, &TreeModel::RRemoveNode);
    connect(sql, &Sqlite::SUpdateMultiLeafTotal, model, &TreeModel::RUpdateMultiLeafTotal);

    connect(sql, &Sqlite::SFreeView, this, &MainWindow::RFreeView);
}

void MainWindow::InsertNode(TreeWidget* tree_widget)
{
    auto current_index { tree_widget->View()->currentIndex() };
    current_index = current_index.isValid() ? current_index : QModelIndex();

    auto parent_index { current_index.parent() };
    parent_index = parent_index.isValid() ? parent_index : QModelIndex();

    const int parent_id { parent_index.isValid() ? parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() : -1 };
    InsertNodeFunction(parent_index, parent_id, current_index.row() + 1);
}

void MainWindow::InsertNodeFunction(const QModelIndex& parent, int parent_id, int row)
{
    auto model { tree_widget_->Model() };

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = model->Rule(parent_id);
    node->unit = model->Unit(parent_id);
    model->SetParent(node, parent_id);

    auto section { data_->info.section };

    if (section == Section::kSales || section == Section::kPurchase)
        InsertNodeOrder(node, parent, row);

    if (section != Section::kSales && section != Section::kPurchase)
        InsertNodeFPTS(node, parent, parent_id, row);
}

void MainWindow::RRemoveTriggered()
{
    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

    if (IsTreeWidget(widget)) {
        assert(dynamic_cast<TreeWidget*>(widget) && "Widget is not TreeWidget");
        RemoveNode(static_cast<TreeWidget*>(widget));
    }

    if (IsTableWidget(widget)) {
        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        RemoveTrans(static_cast<TableWidget*>(widget));
    }
}

void MainWindow::RemoveNode(TreeWidget* tree_widget)
{
    auto view { tree_widget->View() };
    if (!view || !HasSelection(view))
        return;

    auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { tree_widget->Model() };
    if (!model)
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool() };
    const bool is_helper { index.siblingAtColumn(std::to_underlying(TreeEnum::kIsHelper)).data().toBool() };

    if (branch) {
        if (model->ChildrenEmpty(node_id))
            model->RemoveNode(index.row(), index.parent());
        else
            RemoveBranch(model, index, node_id);

        return;
    }

    auto* sql { data_->sql };
    bool interal_reference { sql->InternalReference(node_id) };
    bool exteral_reference { sql->ExternalReference(node_id) };
    bool helper_reference { sql->HelperReferenceFPTS(node_id) };

    if (!interal_reference && !exteral_reference && !helper_reference) {
        RemoveView(model, index, node_id, is_helper);
        return;
    }

    const int unit { index.siblingAtColumn(std::to_underlying(TreeEnum::kUnit)).data().toInt() };

    auto* dialog { new class RemoveNode(model, data_->info.section, node_id, unit, branch, exteral_reference, is_helper, this) };
    connect(dialog, &RemoveNode::SRemoveNode, sql, &Sqlite::RRemoveNode);
    connect(dialog, &RemoveNode::SReplaceNode, sql, &Sqlite::RReplaceNode);
    dialog->exec();
}

void MainWindow::RemoveTrans(TableWidget* table_widget)
{
    // 1. 获取视图和检查选择
    auto view { table_widget->View() };
    if (!view || !HasSelection(view)) {
        return;
    }

    QModelIndex current_index { view->currentIndex() };
    if (!current_index.isValid()) {
        return;
    }

    // 2. 获取模型和当前索引
    auto model { table_widget->Model() };
    if (!model || model->IsHelper()) {
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

void MainWindow::RemoveView(PTreeModel tree_model, const QModelIndex& index, int node_id, bool is_helper)
{
    tree_model->RemoveNode(index.row(), index.parent());
    data_->sql->RemoveNode(node_id, false, is_helper);
    auto* widget { table_hash_->value(node_id) };

    if (widget) {
        FreeWidget(widget);
        table_hash_->remove(node_id);
        SignalStation::Instance().DeregisterModel(data_->info.section, node_id);
    }
}

void MainWindow::SaveTab(CTableHash& table_hash, CString& section_name, CString& property) const
{
    auto keys { table_hash.keys() };
    QStringList list {};

    for (int node_id : keys)
        list.emplaceBack(QString::number(node_id));

    exclusive_interface_->setValue(QString("%1/%2").arg(section_name, property), list);
}

void MainWindow::RestoreTab(PTreeModel tree_model, TableHash& table_hash, CData& data, CSettings& settings, CString& property)
{
    // order不恢复
    Section section { data.info.section };
    if (section == Section::kSales || section == Section::kPurchase)
        return;

    auto variant { exclusive_interface_->value(QString("%1/%2").arg(data.info.node, property)) };

    QSet<int> list {};

    if (variant.isValid() && variant.canConvert<QStringList>()) {
        auto variant_list { variant.value<QStringList>() };
        for (CString& node_id : variant_list)
            list.insert(node_id.toInt());
    }

    for (int node_id : list) {
        if (tree_model->Contains(node_id) && !tree_model->BranchFPTS(node_id) && node_id >= 1) {
            if (tree_model->IsHelperFPTS(node_id))
                CreateTableHelper(tree_model, &table_hash, &data, &settings, node_id);
            else
                CreateTableFPTS(tree_model, &table_hash, &data, &settings, node_id);
        }
    }
}

void MainWindow::Recent()
{
    recent_list_ = shared_interface_->value(RECENT_FILE).toStringList();

    auto* recent_menu { ui->menuRecent };
    QStringList valid_list {};

    for (CString& file : recent_list_) {
        if (QFile::exists(file)) {
            auto* action { recent_menu->addAction(file) };
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

void MainWindow::RemoveBranch(PTreeModel tree_model, const QModelIndex& index, int node_id)
{
    QMessageBox msg {};
    msg.setIcon(QMessageBox::Question);
    msg.setText(tr("Remove %1").arg(tree_model->GetPath(node_id)));
    msg.setInformativeText(tr("The branch will be removed, and its direct children will be promoted to the same level."));
    msg.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

    if (msg.exec() == QMessageBox::Ok)
        tree_model->RemoveNode(index.row(), index.parent());
}

void MainWindow::RTabCloseRequested(int index)
{
    if (index == 0)
        return;

    int node_id { ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id };
    auto* widget { table_hash_->value(node_id) };

    FreeWidget(widget);
    table_hash_->remove(node_id);

    SignalStation::Instance().DeregisterModel(data_->info.section, node_id);
}

void MainWindow::SetTabWidget()
{
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };

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

void MainWindow::SetView(PQTableView view) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::CurrentChanged);
    view->setColumnHidden(std::to_underlying(TableEnum::kID), false);

    auto* h_header { view->horizontalHeader() };
    h_header->setSectionResizeMode(QHeaderView::ResizeToContents);
    h_header->setSectionResizeMode(std::to_underlying(TableEnum::kDescription), QHeaderView::Stretch);

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(ROW_HEIGHT);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);

    view->scrollToBottom();
    view->setCurrentIndex(QModelIndex());
    view->sortByColumn(std::to_underlying(TableEnum::kDateTime), Qt::AscendingOrder); // will run function: AccumulateSubtotal while sorting
}

void MainWindow::SetHelperView(PQTableView view) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::CurrentChanged);
    view->setColumnHidden(std::to_underlying(TableEnumHelper::kID), false);

    auto* h_header { view->horizontalHeader() };
    h_header->setSectionResizeMode(QHeaderView::ResizeToContents);
    h_header->setSectionResizeMode(std::to_underlying(TableEnumHelper::kDescription), QHeaderView::Stretch);

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(ROW_HEIGHT);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);

    view->scrollToBottom();
    view->setCurrentIndex(QModelIndex());
    view->sortByColumn(std::to_underlying(TableEnum::kDateTime), Qt::AscendingOrder);
}

void MainWindow::DelegateHelper(PQTableView table_view, PTreeModel tree_model, CSettings* settings) const
{
    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kCode), line);

    auto* date_time { new TableDateTime(interface_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kDateTime), date_time);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kState), state);

    auto* document { new TableDbClick(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kDocument), document);
    connect(document, &TableDbClick::SEdit, this, &MainWindow::REditDocument);

    auto* value { new TableDoubleSpinR(settings->amount_decimal, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kLhsDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kRhsDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kLhsCredit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kRhsCredit), value);

    auto* ratio { new TableDoubleSpinR(settings->common_decimal, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kLhsRatio), ratio);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kRhsRatio), ratio);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kUnitPrice), ratio);

    auto* node_name { new SearchPathTableR(tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kLhsNode), node_name);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumHelper::kRhsNode), node_name);
}

void MainWindow::SetConnect() const
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

    info.section = section;
    info.node = FINANCE;
    info.path = FINANCE_PATH;
    info.transaction = FINANCE_TRANSACTION;

    QStringList unit_list { "CNY", "HKD", "USD", "GBP", "JPY", "CAD", "AUD", "EUR" };
    auto& unit_map { info.unit_map };

    QStringList unit_symbol_list { "¥", "$", "$", "£", "¥", "$", "$", "€" };
    auto& unit_symbol_map { info.unit_symbol_map };

    for (int i = 0; i != unit_list.size(); ++i) {
        unit_map.insert(i, unit_list.at(i));
        unit_symbol_map.insert(i, unit_symbol_list.at(i));
    }

    sql_.QuerySettings(finance_settings_, section);

    sql = new SqliteFinance(info, this);

    auto* model { new TreeModelFinance(sql, info, finance_settings_.default_unit, finance_table_hash_, interface_.separator, this) };
    finance_tree_ = new TreeWidgetFPT(model, info, finance_settings_, this);

    connect(sql, &Sqlite::SMoveMultiHelperTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiHelperTransFPTS);
}

void MainWindow::SetProductData()
{
    auto section { Section::kProduct };
    auto& info { product_data_.info };
    auto& sql { product_data_.sql };

    info.section = section;
    info.node = PRODUCT;
    info.path = PRODUCT_PATH;
    info.transaction = PRODUCT_TRANSACTION;

    // POS: Position, PC: Piece, SF: SquareFeet
    QStringList unit_list { {}, tr("POS"), tr("BOX"), tr("PC"), tr("SET"), tr("SF") };
    auto& unit_map { info.unit_map };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_map.insert(i, unit_list.at(i));

    sql_.QuerySettings(product_settings_, section);

    sql = new SqliteProduct(info, this);

    auto* model { new TreeModelProduct(sql, info, product_settings_.default_unit, product_table_hash_, interface_.separator, this) };
    product_tree_ = new TreeWidgetFPT(model, info, product_settings_, this);

    connect(sql, &Sqlite::SMoveMultiHelperTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiHelperTransFPTS);
}

void MainWindow::SetStakeholderData()
{
    auto section { Section::kStakeholder };
    auto& info { stakeholder_data_.info };
    auto& sql { stakeholder_data_.sql };

    info.section = section;
    info.node = STAKEHOLDER;
    info.path = STAKEHOLDER_PATH;
    info.transaction = STAKEHOLDER_TRANSACTION;

    // IM：Immediate, MS（Monthly Settlement）
    info.rule_map.insert(0, tr("IM"));
    info.rule_map.insert(1, tr("MS"));

    // EMP: EMPLOYEE, CUST: CUSTOMER, VEND: VENDOR, PROD: PRODUCT
    QStringList unit_list { tr("CUST"), tr("EMP"), tr("VEND"), tr("PROD") };
    auto& unit_map { info.unit_map };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_map.insert(i, unit_list.at(i));

    sql_.QuerySettings(stakeholder_settings_, section);

    sql = new SqliteStakeholder(info, this);

    auto* model { new TreeModelStakeholder(sql, info, stakeholder_settings_.default_unit, stakeholder_table_hash_, interface_.separator, this) };
    stakeholder_tree_ = new TreeWidgetStakeholder(model, info, stakeholder_settings_, this);

    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(sql, &Sqlite::SUpdateStakeholder, model, &TreeModel::RUpdateStakeholder);
    connect(static_cast<SqliteStakeholder*>(sql), &SqliteStakeholder::SAppendPrice, &SignalStation::Instance(), &SignalStation::RAppendPrice);
    connect(sql, &Sqlite::SMoveMultiHelperTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiHelperTransFPTS);
}

void MainWindow::SetTaskData()
{
    auto section { Section::kTask };
    auto& info { task_data_.info };
    auto& sql { task_data_.sql };

    info.section = section;
    info.node = TASK;
    info.path = TASK_PATH;
    info.transaction = TASK_TRANSACTION;

    // PROD: PRODUCT, STKH: STAKEHOLDER
    QStringList unit_list { tr("CUST"), tr("EMP"), tr("VEND"), tr("PROD") };
    auto& unit_map { info.unit_map };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_map.insert(i, unit_list.at(i));

    sql_.QuerySettings(task_settings_, section);

    sql = new SqliteTask(info, this);

    auto* model { new TreeModelTask(sql, info, task_settings_.default_unit, task_table_hash_, interface_.separator, this) };
    task_tree_ = new TreeWidgetFPT(model, info, task_settings_, this);
    connect(sql, &Sqlite::SMoveMultiHelperTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiHelperTransFPTS);
}

void MainWindow::SetSalesData()
{
    auto section { Section::kSales };
    auto& info { sales_data_.info };
    auto& sql { sales_data_.sql };

    info.section = section;
    info.node = SALES;
    info.path = SALES_PATH;
    info.transaction = SALES_TRANSACTION;

    // SO: SALES ORDER, RO: REFUND ORDER
    info.rule_map.insert(0, tr("SO"));
    info.rule_map.insert(1, tr("RO"));

    // IM: IMMEDIATE, MS: MONTHLY SETTLEMENT, PEND: PENDING
    QStringList unit_list { tr("IM"), tr("MS"), tr("PEND") };
    auto& unit_map { info.unit_map };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_map.insert(i, unit_list.at(i));

    sql_.QuerySettings(sales_settings_, section);

    sql = new SqliteOrder(info, this);

    auto* model { new TreeModelOrder(sql, info, sales_settings_.default_unit, sales_table_hash_, interface_.separator, this) };
    sales_tree_ = new TreeWidgetOrder(model, info, sales_settings_, this);

    connect(stakeholder_data_.sql, &Sqlite::SUpdateStakeholder, model, &TreeModel::RUpdateStakeholder);
    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
}

void MainWindow::SetPurchaseData()
{
    auto section { Section::kPurchase };
    auto& info { purchase_data_.info };
    auto& sql { purchase_data_.sql };

    info.section = section;
    info.node = PURCHASE;
    info.path = PURCHASE_PATH;
    info.transaction = PURCHASE_TRANSACTION;

    // PO: PURCHASE ORDER, RO: REFUND ORDER
    info.rule_map.insert(0, tr("PO"));
    info.rule_map.insert(1, tr("RO"));

    // IM: IMMEDIATE, MS: MONTHLY SETTLEMENT, PEND: PENDING
    QStringList unit_list { tr("IM"), tr("MS"), tr("PEND") };

    auto& unit_map { info.unit_map };

    for (int i = 0; i != unit_list.size(); ++i)
        unit_map.insert(i, unit_list.at(i));

    sql_.QuerySettings(purchase_settings_, section);

    sql = new SqliteOrder(info, this);

    auto* model { new TreeModelOrder(sql, info, purchase_settings_.default_unit, purchase_table_hash_, interface_.separator, this) };
    purchase_tree_ = new TreeWidgetOrder(model, info, purchase_settings_, this);

    connect(stakeholder_data_.sql, &Sqlite::SUpdateStakeholder, model, &TreeModel::RUpdateStakeholder);
    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
}

void MainWindow::SetHeader()
{
    finance_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("IsHelper"),
        tr("Foreign Total"), tr("Local Total"), {} };
    finance_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("FXRate"), tr("Code"), tr("Description"), tr("HelperNode"), tr("D"), tr("S"),
        tr("RelatedNode"), tr("Debit"), tr("Credit"), tr("Subtotal") };
    finance_data_.info.search_trans_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("LhsNode"), tr("LhsFXRate"), tr("LhsDebit"), tr("LhsCredit"),
        tr("Description"), {}, {}, {}, {}, tr("D"), tr("S"), tr("RhsCredit"), tr("RhsDebit"), tr("RhsFXRate"), tr("RhsNode") };
    finance_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {}, {}, {},
        {}, {}, {}, {}, {}, tr("Foreign Total"), tr("Local Total") };
    finance_data_.info.helper_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("LhsNode"), tr("LhsRatio"), tr("LhsDebit"), tr("LhsCredit"),
        tr("Description"), tr("UnitPrice"), tr("D"), tr("S"), tr("RhsCredit"), tr("RhsDebit"), tr("RhsRatio"), tr("RhsNode") };

    product_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("IsHelper"),
        tr("Color"), tr("UnitPrice"), tr("Commission"), tr("Quantity"), tr("Amount"), {} };
    product_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("UnitCost"), tr("Code"), tr("Description"), tr("HelperNode"), tr("D"), tr("S"),
        tr("RelatedNode"), tr("Debit"), tr("Credit"), tr("Subtotal") };
    product_data_.info.search_trans_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("LhsNode"), {}, tr("LhsDebit"), tr("LhsCredit"), tr("Description"),
        tr("UnitCost"), {}, {}, {}, tr("D"), tr("S"), tr("RhsCredit"), tr("RhsDebit"), {}, tr("RhsNode") };
    product_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {}, {}, {},
        tr("Color"), tr("UnitPrice"), tr("Commission"), {}, {}, tr("Quantity"), tr("Amount") };
    product_data_.info.helper_header = finance_data_.info.helper_header;

    stakeholder_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"),
        tr("IsHelper"), tr("Deadline"), tr("Employee"), tr("PaymentPeriod"), tr("TaxRate"), {} };
    stakeholder_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("UnitPrice"), tr("Code"), tr("Description"), tr("OutsideProduct"), tr("D"), tr("S"),
        tr("InsideProduct"), tr("PlaceHolder") };
    stakeholder_data_.info.search_trans_header = { tr("ID"), tr("DateTime"), tr("Code"), tr("InsideProduct"), {}, {}, {}, tr("Description"), tr("UnitPrice"),
        tr("NodeID"), {}, {}, tr("D"), tr("S"), {}, {}, {}, tr("OutsideProduct") };
    stakeholder_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {},
        tr("Employee"), tr("Deadline"), {}, tr("PaymentPeriod"), tr("TaxRate"), {}, {}, {}, {} };
    stakeholder_data_.info.helper_header = finance_data_.info.helper_header;

    task_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("IsHelper"),
        tr("DateTime"), tr("Finished"), tr("Color"), tr("UnitCost"), tr("Quantity"), tr("Amount"), {} };
    task_data_.info.table_header = { tr("ID"), tr("DateTime"), tr("UnitCost"), tr("Code"), tr("Description"), tr("HelperNode"), tr("D"), tr("S"),
        tr("RelatedNode"), tr("Debit"), tr("Credit"), tr("Subtotal") };
    task_data_.info.search_trans_header = product_data_.info.search_trans_header;
    task_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), {}, {},
        tr("DateTime"), tr("Color"), tr("UnitCost"), {}, {}, {}, tr("Quantity"), tr("Amount") };
    task_data_.info.helper_header = finance_data_.info.helper_header;

    sales_data_.info.tree_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Party"),
        tr("Employee"), tr("DateTime"), tr("First"), tr("Second"), tr("Finished"), tr("Amount"), tr("Discount"), tr("Settled"), {} };
    sales_data_.info.table_header = { tr("ID"), tr("InsideProduct"), tr("UnitPrice"), tr("Code"), tr("Description"), tr("OutsideProduct"), tr("Color"),
        tr("LhsNode"), tr("First"), tr("Second"), tr("Amount"), tr("DiscountPrice"), tr("Discount"), tr("Settled") };
    sales_data_.info.search_trans_header = { tr("ID"), {}, tr("Code"), tr("InsideProduct"), {}, tr("First"), tr("Second"), tr("Description"), tr("UnitPrice"),
        tr("LhsNode"), tr("DiscountPrice"), tr("Settled"), {}, {}, tr("Amount"), tr("Discount"), {}, tr("OutsideProduct") };
    sales_data_.info.search_node_header = { tr("Name"), tr("ID"), tr("Code"), tr("Description"), tr("Note"), tr("Rule"), tr("Branch"), tr("Unit"), tr("Party"),
        tr("Employee"), tr("DateTime"), {}, tr("First"), tr("Second"), tr("Finished"), tr("Amount"), tr("Discount"), tr("Settled") };

    purchase_data_.info.tree_header = sales_data_.info.tree_header;
    purchase_data_.info.table_header = sales_data_.info.table_header;
    purchase_data_.info.search_trans_header = sales_data_.info.search_trans_header;
    purchase_data_.info.search_node_header = sales_data_.info.search_node_header;
}

void MainWindow::SetAction() const
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

void MainWindow::SetView(PQTreeView tree_view) const
{
    tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_view->setDragDropMode(QAbstractItemView::InternalMove);
    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked);
    tree_view->setDropIndicatorShown(true);
    tree_view->setSortingEnabled(true);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->setExpandsOnDoubleClick(true);
    tree_view->setColumnHidden(std::to_underlying(TreeEnum::kID), false);

    auto* header { tree_view->header() };
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
    header->setStretchLastSection(true);
    header->setDefaultAlignment(Qt::AlignCenter);
}

void MainWindow::RAppendNodeTriggered()
{
    auto* current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTreeWidget(current_widget))
        return;

    auto view { tree_widget_->View() };
    if (!HasSelection(view))
        return;

    auto parent_index { view->currentIndex() };
    if (!parent_index.isValid())
        return;

    const bool branch { parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool() };
    if (!branch)
        return;

    const int parent_id { parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    InsertNodeFunction(parent_index, parent_id, 0);
}

void MainWindow::AppendTrans(TableWidget* table_widget)
{
    if (!table_widget)
        return;

    auto model { table_widget->Model() };
    if (!model || model->IsHelper())
        return;

    constexpr int ID_ZERO = 0;
    const int empty_row = model->GetNodeRow(ID_ZERO);

    QModelIndex target_index {};

    if (empty_row == -1) {
        const int new_row = model->rowCount();
        if (!model->insertRows(new_row, 1))
            return;

        target_index = model->index(new_row, std::to_underlying(TableEnum::kDateTime));
    } else {
        target_index = model->index(empty_row, std::to_underlying(TableEnum::kRhsNode));
    }

    table_widget->View()->setCurrentIndex(target_index);
}

void MainWindow::RJumpTriggered()
{
    auto* current_widget { ui->tabWidget->currentWidget() };
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
        CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, rhs_node_id);

    const int trans_id { index.sibling(row, std::to_underlying(TableEnum::kID)).data().toInt() };
    SwitchTab(rhs_node_id, trans_id);
}

void MainWindow::RTreeViewCustomContextMenuRequested(const QPoint& pos)
{
    Q_UNUSED(pos);

    auto* menu = new QMenu(this);
    menu->addAction(ui->actionInsert);
    menu->addAction(ui->actionEdit);
    menu->addAction(ui->actionAppendNode);
    menu->addAction(ui->actionRemove);

    menu->exec(QCursor::pos());
}

void MainWindow::REditNode()
{
    auto section { data_->info.section };
    if (section == Section::kSales || section == Section::kPurchase)
        return;

    const auto* widget { ui->tabWidget->currentWidget() };
    if (!widget || !IsTreeWidget(widget))
        return;

    const auto view { tree_widget_->View() };
    if (!HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    EditNodeFPTS(index, node_id);
}

void MainWindow::EditNodeFPTS(const QModelIndex& index, int node_id)
{
    auto section { data_->info.section };
    const auto& unit_map { data_->info.unit_map };

    QDialog* dialog {};
    auto model { tree_widget_->Model() };

    auto* tmp_node { ResourcePool<Node>::Instance().Allocate() };
    model->CopyNodeFPTS(tmp_node, node_id);

    const auto& parent { index.parent() };
    const int parent_id { parent.isValid() ? parent.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() : -1 };
    auto parent_path { model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& name_list { model->ChildrenNameFPTS(parent_id, node_id) };
    const auto* sqlite { data_->sql };

    bool is_not_referenced { !sqlite->InternalReference(node_id) && !sqlite->ExternalReference(node_id) };
    bool branch_enable { is_not_referenced && model->ChildrenEmpty(node_id) && !table_hash_->contains(node_id) };
    bool unit_enable { is_not_referenced };

    switch (section) {
    case Section::kFinance:
        dialog = new EditNodeFinance(tmp_node, unit_map, parent_path, name_list, branch_enable, unit_enable, this);
        break;
    case Section::kTask:
        dialog = new EditNodeFinance(tmp_node, unit_map, parent_path, name_list, branch_enable, unit_enable, this);
        break;
    case Section::kStakeholder:
        unit_enable = is_not_referenced && model->ChildrenEmpty(node_id);
        dialog = new EditNodeStakeholder(tmp_node, unit_map, parent_path, name_list, branch_enable, unit_enable, settings_->amount_decimal, model, this);
        break;
    case Section::kProduct:
        unit_enable = is_not_referenced && model->ChildrenEmpty(node_id);
        dialog = new EditNodeProduct(tmp_node, unit_map, parent_path, name_list, branch_enable, unit_enable, settings_->amount_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(tmp_node);
    }

    connect(dialog, &QDialog::accepted, this, [=]() { model->UpdateNodeFPTS(tmp_node); });
    dialog->exec();
    ResourcePool<Node>::Instance().Recycle(tmp_node);
}

void MainWindow::InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row)
{
    auto tree_model { tree_widget_->Model() };
    auto section { data_->info.section };

    auto parent_path { tree_model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& info { data_->info };
    const auto& name_list { tree_model->ChildrenNameFPTS(parent_id, 0) };

    QDialog* dialog {};

    switch (section) {
    case Section::kFinance:
        dialog = new EditNodeFinance(node, info.unit_map, parent_path, name_list, true, true, this);
        break;
    case Section::kTask:
        node->date_time = QDateTime::currentDateTime().toString(DATE_TIME_FST);
        dialog = new EditNodeFinance(node, info.unit_map, parent_path, name_list, true, true, this);
        break;
    case Section::kStakeholder:
        node->unit = settings_->default_unit;
        dialog = new EditNodeStakeholder(node, info.unit_map, parent_path, name_list, true, true, settings_->common_decimal, tree_model, this);
        break;
    case Section::kProduct:
        dialog = new EditNodeProduct(node, info.unit_map, parent_path, name_list, true, true, settings_->common_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(node);
    }

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (tree_model->InsertNode(row, parent, node)) {
            auto index = tree_model->index(row, 0, parent);
            tree_widget_->View()->setCurrentIndex(index);
        }
    });

    connect(dialog, &QDialog::rejected, this, [=]() { ResourcePool<Node>::Instance().Recycle(node); });
    dialog->exec();
}

void MainWindow::InsertNodeOrder(Node* node, const QModelIndex& parent, int row)
{
    auto tree_model { tree_widget_->Model() };

    auto* node_shadow { ResourcePool<NodeShadow>::Instance().Allocate() };
    tree_model->SetNodeShadowOrder(node_shadow, node);
    if (!node_shadow->id)
        return;

    EditNodeOrder* dialog {};
    auto section { data_->info.section };
    auto* sql { data_->sql };

    auto* table_model { new TableModelOrder(sql, node->rule, 0, data_->info, node_shadow, product_tree_->Model(), stakeholder_data_.sql, this) };

    switch (section) {
    case Section::kSales:
        dialog = new EditNodeOrder(node_shadow, sql, table_model, stakeholder_tree_->Model(), *settings_, UNIT_CUST, this);
        dialog->setWindowTitle(tr(Sales));
        break;
    case Section::kPurchase:
        dialog = new EditNodeOrder(node_shadow, sql, table_model, stakeholder_tree_->Model(), *settings_, UNIT_VEND, this);
        dialog->setWindowTitle(tr(Purchase));
        break;
    default:
        ResourcePool<NodeShadow>::Instance().Recycle(node_shadow);
        ResourcePool<Node>::Instance().Recycle(node);
        return;
    }

    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (tree_model->InsertNode(row, parent, node)) {
            auto index = tree_model->index(row, 0, parent);
            tree_widget_->View()->setCurrentIndex(index);
            dialog_hash_->insert(node->id, dialog);
            dialog_list_->removeOne(dialog);
        }
    });
    connect(dialog, &QDialog::rejected, this, [=, this]() {
        ResourcePool<NodeShadow>::Instance().Recycle(node_shadow);
        if (node->id == 0) {
            ResourcePool<Node>::Instance().Recycle(node);
            dialog_list_->removeOne(dialog);
        } else {
            dialog_hash_->remove(node->id);
        }
    });

    connect(stakeholder_tree_->Model(), &TreeModelStakeholder::SUpdateComboModel, dialog, &EditNodeOrder::RUpdateComboModel);
    connect(table_model, &TableModel::SResizeColumnToContents, dialog->View(), &QTableView::resizeColumnToContents);

    connect(table_model, &TableModel::SUpdateLeafValue, tree_model, &TreeModel::RUpdateLeafValue);
    connect(table_model, &TableModel::SUpdateLeafValueOne, tree_model, &TreeModel::RUpdateLeafValueOne);

    connect(table_model, &TableModel::SUpdateLeafValue, dialog, &EditNodeOrder::RUpdateLeafValue);
    connect(table_model, &TableModel::SUpdateLeafValueOne, dialog, &EditNodeOrder::RUpdateLeafValueOne);

    assert(dynamic_cast<TreeModelOrder*>(tree_widget_->Model().data()) && "Model is not TreeModelOrder");
    auto* tree_model_order { static_cast<TreeModelOrder*>(tree_model.data()) };
    connect(tree_model_order, &TreeModelOrder::SUpdateData, dialog, &EditNodeOrder::RUpdateData);
    connect(dialog, &EditNodeOrder::SUpdateFinished, tree_model_order, &TreeModelOrder::RUpdateFinished);
    connect(dialog, &EditNodeOrder::SUpdateFinished, table_model, &TableModelOrder::RUpdateFinished);
    connect(dialog, &EditNodeOrder::SUpdateNodeID, table_model, &TableModelOrder::RUpdateNodeID);
    connect(dialog, &EditNodeOrder::SUpdateParty, table_model, &TableModelOrder::RUpdateParty);

    dialog_list_->append(dialog);

    SetView(dialog->View());
    DelegateOrder(dialog->View(), settings_);
    dialog->show();
}

void MainWindow::REditDocument()
{
    auto* current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTableWidget(current_widget))
        return;

    auto view { GetQTableView(current_widget) };
    if (!HasSelection(view))
        return;

    auto trans_index { view->currentIndex() };
    if (!trans_index.isValid())
        return;

    auto document_dir { settings_->document_dir };
    auto model { GetTableModel(current_widget) };
    auto* document_pointer { model->GetDocumentPointer(trans_index) };
    const int trans_id { trans_index.siblingAtColumn(std::to_underlying(TableEnum::kID)).data().toInt() };

    auto* dialog { new EditDocument(data_->info.section, document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->UpdateField(data_->info.transaction, document_pointer->join(SEMICOLON), DOCUMENT, trans_id);
}

void MainWindow::RUpdateName(int node_id, CString& name, bool branch)
{
    auto model { tree_widget_->Model() };
    auto* widget { ui->tabWidget };

    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    QSet<int> nodes;

    if (!branch) {
        nodes.insert(node_id);
        if (data_->info.section == Section::kStakeholder)
            UpdateStakeholderReference(nodes, branch);

        if (!table_hash_->contains(node_id))
            return;

    } else {
        nodes = model->ChildrenIDFPTS(node_id);
    }

    int tab_node_id {};
    QString path {};

    for (int index = 0; index != count; ++index) {
        tab_node_id = tab_bar->tabData(index).value<Tab>().node_id;

        if (widget->isTabVisible(index) && nodes.contains(tab_node_id)) {
            path = model->GetPath(tab_node_id);

            if (!branch) {
                tab_bar->setTabText(index, name);
            }

            tab_bar->setTabToolTip(index, path);
        }
    }

    if (data_->info.section == Section::kStakeholder)
        UpdateStakeholderReference(nodes, branch);
}

void MainWindow::RUpdateSettings(CSettings& settings, CInterface& interface)
{
    bool resize_column { false };

    if (interface_ != interface) {
        resize_column |= interface_.date_format != interface.date_format;
        UpdateInterface(interface);
    }

    if (*settings_ != settings) {
        bool update_default_unit { settings_->default_unit != settings.default_unit };
        resize_column |= settings_->amount_decimal != settings.amount_decimal || settings_->common_decimal != settings.common_decimal;

        *settings_ = settings;

        if (update_default_unit)
            tree_widget_->Model()->UpdateDefaultUnit(settings.default_unit);

        tree_widget_->SetStatus();
        sql_.UpdateSettings(settings, data_->info.section);
    }

    if (resize_column) {
        auto* current_widget { ui->tabWidget->currentWidget() };
        if (IsTableWidget(current_widget))
            ResizeColumn(GetQTableView(current_widget)->horizontalHeader(), true);
        if (IsTreeWidget(current_widget))
            ResizeColumn(tree_widget_->View()->header(), false);
    }
}
void MainWindow::RFreeView(int node_id)
{
    auto* view { table_hash_->value(node_id) };

    if (view) {
        FreeWidget(view);
        table_hash_->remove(node_id);
        SignalStation::Instance().DeregisterModel(data_->info.section, node_id);
    }
}

void MainWindow::UpdateInterface(CInterface& interface)
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
        finance_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);
        stakeholder_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);
        product_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);
        task_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);

        auto* widget { ui->tabWidget };
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

void MainWindow::UpdateTranslate() const
{
    QWidget* widget {};
    Tab tab_id {};
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };
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

void MainWindow::UpdateRecent() const { shared_interface_->setValue(RECENT_FILE, recent_list_); }

void MainWindow::UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const
{
    auto* widget { ui->tabWidget };
    auto stakeholder_model { tree_widget_->Model() };
    auto* order_model { static_cast<TreeModelOrder*>(sales_tree_->Model().data()) };
    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    // 使用 QtConcurrent::run 启动后台线程
    auto future = QtConcurrent::run([=]() -> QVector<std::tuple<int, QString, QString>> {
        QVector<std::tuple<int, QString, QString>> updates;

        // 遍历所有选项卡，计算需要更新的项
        for (int index = 0; index != count; ++index) {
            const auto& data { tab_bar->tabData(index).value<Tab>() };
            bool update = data.section == Section::kSales || data.section == Section::kPurchase;

            if (!widget->isTabVisible(index) && update) {
                int order_node_id = data.node_id;
                if (order_node_id == 0)
                    continue;

                int order_party = order_model->Party(order_node_id);
                if (!stakeholder_nodes.contains(order_party))
                    continue;

                QString name = stakeholder_model->Name(order_party);
                QString path = stakeholder_model->GetPath(order_party);

                // 收集需要更新的信息
                updates.append(std::make_tuple(index, name, path));
            }
        }

        return updates;
    });

    // 创建 QFutureWatcher 用于监控任务完成
    auto* watcher = new QFutureWatcher<QVector<std::tuple<int, QString, QString>>>();

    // 连接信号槽，监测任务完成
    connect(watcher, &QFutureWatcher<QVector<std::tuple<int, QString, QString>>>::finished, this, [watcher, tab_bar, branch]() {
        // 获取后台线程的结果
        const auto& updates = watcher->result();

        // 更新 UI
        for (const auto& [index, name, path] : updates) {
            if (!branch)
                tab_bar->setTabText(index, name);

            tab_bar->setTabToolTip(index, path);
        }

        // 删除 watcher，避免内存泄漏
        watcher->deleteLater();
    });

    // 设置未来任务给 watcher
    watcher->setFuture(future);
}

void MainWindow::LoadAndInstallTranslator(CString& language)
{
    QString cash_language { QString(":/I18N/I18N/") + YTX + "_" + language + SFX_QM };
    if (cash_translator_.load(cash_language))
        qApp->installTranslator(&cash_translator_);

    QString base_language { ":/I18N/I18N/qtbase_" + language + SFX_QM };
    if (base_translator_.load(base_language))
        qApp->installTranslator(&base_translator_);
}

void MainWindow::ResizeColumn(QHeaderView* header, bool table_view) const
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_view ? header->setSectionResizeMode(std::to_underlying(TableEnum::kDescription), QHeaderView::Stretch)
               : header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
    ;
}

void MainWindow::SharedInterface(CString& dir_path)
{
    static QSettings shared_interface(dir_path + SLASH + YTX + SFX_INI, QSettings::IniFormat);
    shared_interface_ = &shared_interface;

    QString language_code {};

    switch (QLocale::system().language()) {
    case QLocale::English:
        language_code = EN_US;
        break;
    case QLocale::Chinese:
        language_code = ZH_CN;
        break;
    default:
        language_code = EN_US;
        break;
    }

    shared_interface.beginGroup(INTERFACE);
    interface_.language = shared_interface.value(LANGUAGE, language_code).toString();
    interface_.theme = shared_interface.value(THEME, SOLARIZED_DARK).toString();
    interface_.date_format = shared_interface.value(DATE_FORMAT, DATE_TIME_FST).toString();
    interface_.separator = shared_interface.value(SEPARATOR, DASH).toString();
    shared_interface.endGroup();

    LoadAndInstallTranslator(interface_.language);

#ifdef Q_OS_WIN
    QString theme { "file:///:/theme/theme/" + interface_.theme + " Win" + SFX_QSS };
#elif defined(Q_OS_MACOS)
    QString theme { "file:///:/theme/theme/" + interface_.theme + " Mac" + SFX_QSS };
#endif

    qApp->setStyleSheet(theme);

    QTimer::singleShot(0, this, []() {
        StyledItemDelegate::SetTextMargin();
        StyledItemDelegate::SetFontMetrics();
    });
}

void MainWindow::ExclusiveInterface(CString& dir_path, CString& base_name)
{
    static QSettings exclusive_interface(dir_path + SLASH + base_name + SFX_INI, QSettings::IniFormat);
    exclusive_interface_ = &exclusive_interface;
}

void MainWindow::ResourceFile() const
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
    QString command { "E:/Qt/6.8.0/llvm-mingw_64/bin/rcc.exe" };
    QStringList arguments {};
    arguments << "-binary"
              << "E:/Code/YTX/resource/resource.qrc"
              << "-o" << path;

    QProcess process {};

    // 启动终端并执行命令
    process.start(command, arguments);
    process.waitForFinished();
#endif

#elif defined(Q_OS_MACOS)
    path = QCoreApplication::applicationDirPath() + "/../Resources/resource.brc";

#if 0
    QString command { QDir::homePath() + "/Qt6.8/6.8.0/macos/libexec/rcc" + " -binary " + QDir::homePath() + "/Documents/YTX/resource/resource.qrc -o "
        + path };

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

    auto* dialog { new Search(
        data_->info, tree_widget_->Model(), stakeholder_tree_->Model(), product_tree_->Model(), data_->sql, data_->info.rule_map, *settings_, this) };
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    connect(dialog, &Search::STreeLocation, this, &MainWindow::RTreeLocation);
    connect(dialog, &Search::STableLocation, this, &MainWindow::RTableLocation);
    connect(tree_widget_->Model(), &TreeModel::SSearch, dialog, &Search::RSearch);
    connect(dialog, &QDialog::rejected, this, [=, this]() { dialog_list_->removeOne(dialog); });

    dialog_list_->append(dialog);
    dialog->show();
}

void MainWindow::RTreeLocation(int node_id)
{
    auto* widget { tree_widget_ };
    ui->tabWidget->setCurrentWidget(widget);

    tree_widget_->Model()->RetriveNodeOrder(node_id);
    auto index { tree_widget_->Model()->GetIndex(node_id) };
    widget->activateWindow();
    widget->View()->setCurrentIndex(index);
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
        CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, id);

    SwitchTab(id, trans_id);
}

void MainWindow::RUpdateParty(int node_id, int party_id)
{
    auto model { stakeholder_tree_->Model() };
    auto* widget { ui->tabWidget };
    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    for (int index = 0; index != count; ++index) {
        if (widget->isTabVisible(index) && tab_bar->tabData(index).value<Tab>().node_id == node_id) {
            tab_bar->setTabText(index, model->Name(party_id));
            tab_bar->setTabToolTip(index, model->GetPath(party_id));
        }
    }
}

void MainWindow::RPreferencesTriggered()
{
    if (!SqlConnection::Instance().DatabaseEnable())
        return;

    auto model { tree_widget_->Model() };

    auto* preference { new Preferences(data_->info, model, interface_, *settings_, this) };
    connect(preference, &Preferences::SUpdateSettings, this, &MainWindow::RUpdateSettings);
    preference->exec();
}

void MainWindow::RAboutTriggered()
{
    static About* dialog = nullptr;

    if (!dialog) {
        dialog = new About(this);
        dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
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
    auto* menu { ui->menuRecent };

    if (!menu->isEmpty()) {
        auto* separator { ui->actionSeparator };
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
    auto* current_widget { ui->tabWidget->currentWidget() };
    if (!current_widget || !IsTableWidget(current_widget))
        return;

    auto table_model { GetTableModel(current_widget) };
    table_model->UpdateAllState(Check { QObject::sender()->property(CHECK).toInt() });
}

void MainWindow::SwitchSection(CTab& last_tab) const
{
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };
    int count { tab_widget->count() };
    Tab tab {};

    for (int index = 0; index != count; ++index) {
        tab = tab_bar->tabData(index).value<Tab>();
        tab.section == start_ ? tab_widget->setTabVisible(index, true) : tab_widget->setTabVisible(index, false);

        if (tab == last_tab)
            tab_widget->setCurrentIndex(index);
    }

    SwitchDialog(dialog_list_, true);
    SwitchDialog(dialog_hash_, true);
}

void MainWindow::UpdateLastTab() const
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
    SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = finance_tree_;
    table_hash_ = &finance_table_hash_;
    dialog_list_ = &finance_dialog_list_;
    dialog_hash_ = &finance_dialog_hash_;
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
    SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = sales_tree_;
    table_hash_ = &sales_table_hash_;
    dialog_list_ = &sales_dialog_list_;
    dialog_hash_ = &sales_dialog_hash_;
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
    SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = task_tree_;
    table_hash_ = &task_table_hash_;
    dialog_list_ = &task_dialog_list_;
    dialog_hash_ = &task_dialog_hash_;
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
    SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = stakeholder_tree_;
    table_hash_ = &stakeholder_table_hash_;
    dialog_list_ = &stakeholder_dialog_list_;
    dialog_hash_ = &stakeholder_dialog_hash_;
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
    SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = product_tree_;
    table_hash_ = &product_table_hash_;
    dialog_list_ = &product_dialog_list_;
    dialog_hash_ = &product_dialog_hash_;
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
    SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = purchase_tree_;
    table_hash_ = &purchase_table_hash_;
    dialog_list_ = &purchase_dialog_list_;
    dialog_hash_ = &purchase_dialog_hash_;
    settings_ = &purchase_settings_;
    data_ = &purchase_data_;

    SwitchSection(data_->tab);
}
