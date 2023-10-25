#include "mainwindow.h"
#include "delegate/alignright.h"
#include "delegate/datedelegate.h"
#include "delegate/documentdelegate.h"
#include "delegate/numericaldelegate.h"
#include "delegate/statedelegate.h"
#include "delegate/textdelegate.h"
#include "delegate/transferdelegate.h"
#include "dialog/editnode.h"
#include "dialog/insertnode.h"
#include "dialog/linkeddocument.h"
#include "globalmanager/nodepool.h"
#include "globalmanager/signalmanager.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto financial_tree_info_ = TreeInfo(kFinancialTable, kFinancialPathTable, kFinancialTransactionTable);
    financial_ = CreateTree(financial_tree_info_);
    ui->tabWidget->addTab(financial_.first, "Financial");
    IniContainer(ui->tabWidget);
    IniConnect();

    date_format_ = "yyyy/MM/dd";
    decimal_ = 2;
    home_dir_ = QDir::homePath();
}

MainWindow::~MainWindow()
{
    delete financial_.second;
    delete financial_.first;
    delete ui;

    for (auto it = opened_table_view_.begin(); it != opened_table_view_.end(); ++it) {
        auto view = it.value();
        FreeView(view);
    }

    opened_table_view_.clear();
}

void MainWindow::do_treeView_doubleClicked(const QModelIndex& index)
{
    if (index.column() != 0)
        return;

    bool placeholder = index.siblingAtColumn(Tree::kPlaceholder).data().toBool();
    if (placeholder)
        return;

    int node_id = index.siblingAtColumn(Tree::kID).data().toInt();
    QString node_name = index.siblingAtColumn(Tree::kName).data().toString();
    bool node_rule = index.siblingAtColumn(Tree::kRule).data().toBool();

    if (opened_table_view_.contains(node_id)) {
        SwitchTab(node_id);
    } else {
        TableInfo info(node_id, node_name, 2, node_rule, kFinancialTransactionTable);

        CreateTable(info, ui->tabWidget, financial_.second);
    }
}

void MainWindow::SwitchTab(int node_id)
{
    auto view = opened_table_view_.value(node_id);
    ui->tabWidget->setCurrentWidget(view);
}

void MainWindow::CreateTable(const TableInfo& info, QTabWidget* container, TreeModel* tree_model)
{
    auto view = new QTableView(container);
    auto model = new TableModel(info, view);
    view->setModel(model);

    int tab_index = container->addTab(view, info.node_name);
    int node_id = info.node_id;
    container->tabBar()->setTabData(tab_index, node_id);
    container->setCurrentWidget(view);

    auto leaf_map = tree_model->GetLeafMap();
    RemoveOneLeaf(leaf_map, info.node_id);

    IniTableView(view);
    IniTableHeaderView(view);
    IniTableConnect(model, tree_model);
    CreateTableDelegate(view, leaf_map, tree_model);

    opened_table_view_.insert(node_id, view);
    SignalManager::Instance().RegisterTableModel(node_id, model);
}

void MainWindow::IniTableConnect(TableModel* table, TreeModel* tree)
{
    connect(table, &TableModel::SendUpdate, &SignalManager::Instance(), &SignalManager::ReceiveUpdate, Qt::UniqueConnection);
    connect(table, &TableModel::SendRemove, &SignalManager::Instance(), &SignalManager::ReceiveRemove, Qt::UniqueConnection);
    connect(table, &TableModel::SendCopy, &SignalManager::Instance(), &SignalManager::ReceiveCopy, Qt::UniqueConnection);

    connect(tree, &TreeModel::SendRule, table, &TableModel::ReceiveRule, Qt::UniqueConnection);
    connect(table, &TableModel::SendReCalculate, tree, &TreeModel::ReceiveReCalculate, Qt::UniqueConnection);
}

void MainWindow::CreateTableDelegate(QTableView* view, const QMultiMap<QString, int>& leaf_map, TreeModel* tree_model)
{
    auto transfer = new TransferDelegate(leaf_map, view);
    view->setItemDelegateForColumn(Table::kTransfer, transfer);
    connect(tree_model, &TreeModel::SendLeaf, transfer, &TransferDelegate::ReceiveLeaf, Qt::UniqueConnection);

    auto numerical = new NumericalDelegate(decimal_, view);
    view->setItemDelegateForColumn(Table::kDebit, numerical);
    view->setItemDelegateForColumn(Table::kCredit, numerical);

    auto date = new DateDelegate(date_format_, view);
    view->setItemDelegateForColumn(Table::kPostDate, date);

    auto description = new TextDelegate(view);
    view->setItemDelegateForColumn(Table::kDescription, description);

    auto align_right = new AlignRight(view);
    view->setItemDelegateForColumn(Table::kBalance, align_right);

    auto statue = new StateDelegate(view);
    view->setItemDelegateForColumn(Table::kStatus, statue);

    auto document = new DocumentDelegate(view);
    view->setItemDelegateForColumn(Table::kDocument, document);
}

QPair<QTreeView*, TreeModel*> MainWindow::CreateTree(const TreeInfo& info)
{
    auto view = new QTreeView(this);
    auto model = new TreeModel(info, view);
    view->setModel(model);

    IniTreeView(view);
    IniTreeHeaderView(view);
    CreateTreeDelegate(view);

    return QPair<QTreeView*, TreeModel*>(view, model);
}

void MainWindow::CreateTreeDelegate(QTreeView* view)
{
    auto text = new TextDelegate(view);
    view->setItemDelegateForColumn(Tree::kDescription, text);
    view->setItemDelegateForColumn(Tree::kRule, text);
    view->setItemDelegateForColumn(Tree::kPlaceholder, text);

    auto align_right = new AlignRight(view);
    view->setItemDelegateForColumn(Tree::kTotal, align_right);
}

void MainWindow::IniTreeHeaderView(QTreeView* view)
{
    auto header = view->header();

    for (int column = 0; column != view->model()->columnCount(); ++column) {
        switch (column) {
        case Tree::kDescription:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        default:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            break;
        }
    }
}

void MainWindow::InsertRowInTree(QTreeView* view, TreeModel* model)
{
    auto current_index = view->currentIndex();
    current_index = current_index.isValid() ? current_index : QModelIndex();

    auto parent_index = current_index.parent();
    parent_index = parent_index.isValid() ? parent_index : QModelIndex();
    bool parent_rule = parent_index.siblingAtColumn(Tree::kRule).data().toBool();

    int row = current_index.isValid() ? current_index.row() + 1 : 0;

    auto node = NodePool::Instance().Allocate();
    node->rule = parent_rule;

    auto dialog = new InsertNode(node, view);
    if (dialog->exec() == QDialog::Rejected)
        return;

    if (model->insertRow(row, parent_index, node)) {
        auto index = model->index(row, 0, parent_index);
        view->setCurrentIndex(index);
    }
}

void MainWindow::InsertRowInTable(QWidget* widget)
{
    auto view = GetTableView(widget);
    if (!view)
        return;

    auto model = GetTableModel(view);
    if (!model)
        return;

    int empty_transfer = model->EmptyTransfer();

    if (empty_transfer == -1) {
        int row = view->currentIndex().isValid() ? view->currentIndex().row() + 1 : 0;
        if (model->insertRow(row)) {
            auto index = model->index(row, Table::kPostDate);
            view->setCurrentIndex(index);
            view->edit(index.sibling(index.row(), Table::kPostDate));
        }
    } else {
        auto index = model->index(empty_transfer, Table::kTransfer);
        view->setCurrentIndex(index);
        view->edit(index.sibling(index.row(), Table::kTransfer));
    }
}

void MainWindow::do_actionDelete()
{
    auto widget = ui->tabWidget->currentWidget();

    if (IsTreeView(widget)) {
        DeleteFromTreeView(financial_.first, financial_.second);
        return;
    }

    if (IsTableView(widget)) {
        DeleteFromTableView(ui->tabWidget->currentWidget());
        return;
    }
}

void MainWindow::do_actionNew()
{
    auto widget = ui->tabWidget->currentWidget();

    if (IsTreeView(widget)) {
        InsertRowInTree(financial_.first, financial_.second);
        return;
    }

    if (IsTableView(widget)) {
        InsertRowInTable(ui->tabWidget->currentWidget());
        return;
    }
}

void MainWindow::DeleteFromTreeView(QTreeView* view, TreeModel* model)
{
    if (!HasSelection(view))
        return;

    auto index = view->currentIndex();
    if (!model || !index.isValid())
        return;

    int node_id = index.siblingAtColumn(Tree::kID).data().toInt();
    auto node = model->GetNode(node_id);
    bool placeholder = index.siblingAtColumn(Tree::kPlaceholder).data().toBool();

    if (placeholder) {
        if (node->children.isEmpty()) {
            model->removeRow(index.row(), index.parent());
            return;
        }

        ShowPlaceholderMsgBox(model, index);
        return;
    }

    if (!model->UsageOfNode(node_id)) {
        DeleteNodeAndRelatedView(model, index, node_id);
        return;
    }

    auto leaf_map = model->GetLeafMap();
    RemoveOneLeaf(leaf_map, node_id);

    auto dialog = new DeleteNode(node_id, leaf_map, view);
    IniDeleteNodeConnect(dialog, model);

    if (dialog->exec() == QDialog::Accepted) {
        DeleteNodeAndRelatedView(model, index, node_id);
    }
}

void MainWindow::IniDeleteNodeConnect(DeleteNode* dialog, TreeModel* model)
{
    connect(dialog, &DeleteNode::SendDelete, model, &TreeModel::ReceiveDelete, Qt::UniqueConnection);
    connect(dialog, &DeleteNode::SendReplace, model, &TreeModel::ReceiveReplace, Qt::UniqueConnection);
    connect(dialog, &DeleteNode::SendReloadAll, &SignalManager::Instance(), &SignalManager::ReceiveReloadAll, Qt::UniqueConnection);
}

void MainWindow::DeleteNodeAndRelatedView(TreeModel* model, const QModelIndex& index, int node_id)
{
    model->removeRow(index.row(), index.parent());
    auto related_view = opened_table_view_.value(node_id);

    if (related_view) {
        FreeView(related_view);
        opened_table_view_.remove(node_id);
        SignalManager::Instance().DeregisterTableModel(node_id);
    }
}

QTableView* MainWindow::GetTableView(QWidget* widget)
{
    return qobject_cast<QTableView*>(widget);
}

TableModel* MainWindow::GetTableModel(QTableView* view)
{
    return qobject_cast<TableModel*>(view->model());
}

bool MainWindow::IsTreeView(QWidget* widget)
{
    return widget->inherits("QTreeView");
}

bool MainWindow::IsTableView(QWidget* widget)
{
    return widget->inherits("QTableView");
}

void MainWindow::RemoveOneLeaf(QMultiMap<QString, int>& leaf_map, int node_id)
{
    for (auto it = leaf_map.begin(); it != leaf_map.end(); ++it) {
        if (it.value() == node_id) {
            leaf_map.remove(it.key());
            return;
        }
    }
}

void MainWindow::ShowPlaceholderMsgBox(TreeModel* model, const QModelIndex& index)
{
    QMessageBox msg_box;
    msg_box.setIcon(QMessageBox::Information);
    msg_box.setText("Remove a placeholder node");
    msg_box.setInformativeText("This node will be deleted, and its children will be promoted to the current layer");

    msg_box.addButton(QMessageBox::Cancel);
    msg_box.addButton(QMessageBox::Ok);

    if (msg_box.exec() == QMessageBox::Ok) {
        model->removeRow(index.row(), index.parent());
    }
}

void MainWindow::DeleteFromTableView(QWidget* widget)
{
    auto view = GetTableView(widget);
    if (!HasSelection(view))
        return;

    auto model = GetTableModel(view);
    auto index = view->currentIndex();

    if (model && index.isValid()) {
        model->removeRow(index.row());
    }
}

void MainWindow::do_releaseView(int index)
{
    int node_id = ui->tabWidget->tabBar()->tabData(index).toInt();
    auto view = opened_table_view_.value(node_id);

    FreeView(view);
    opened_table_view_.remove(node_id);

    SignalManager::Instance().DeregisterTableModel(node_id);
}

void MainWindow::FreeView(QTableView* view)
{
    if (view) {
        auto model = view->model();
        if (model) {
            view->setModel(nullptr);
            delete model;
        }
        delete view;
    }
}

void MainWindow::IniContainer(QTabWidget* container)
{
    container->tabBar()->setDocumentMode(true);
    container->tabBar()->setExpanding(false);
    container->setMovable(true);
    container->setTabsClosable(true);
    container->setElideMode(Qt::ElideNone);
    container->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);
}

void MainWindow::IniTableView(QTableView* view)
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::CurrentChanged);
    view->installEventFilter(this);
}

void MainWindow::IniTableHeaderView(QTableView* view)
{
    auto header = view->horizontalHeader();
    header->setSectionsMovable(false);

    for (int column = 0; column != view->model()->columnCount(); ++column) {
        switch (column) {
        case Table::kDescription:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        default:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            break;
        }
    }
}

void MainWindow::IniConnect()
{
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::do_releaseView, Qt::UniqueConnection);
    connect(financial_.first, &QTreeView::doubleClicked, this, &MainWindow::do_treeView_doubleClicked, Qt::UniqueConnection);
    connect(financial_.first, &QTreeView::customContextMenuRequested, this, &MainWindow::do_treeView_customContextMenuRequested, Qt::UniqueConnection);
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::do_actionNew, Qt::UniqueConnection);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::do_actionDelete, Qt::UniqueConnection);
    connect(ui->actionAppend, &QAction::triggered, this, &MainWindow::do_actionAppend, Qt::UniqueConnection);
    connect(ui->actionJump, &QAction::triggered, this, &MainWindow::do_actionJump, Qt::UniqueConnection);
    connect(ui->actionEditNode, &QAction::triggered, this, &MainWindow::do_actionEditNode, Qt::UniqueConnection);
    connect(ui->actionLinkedDocument, &QAction::triggered, this, &MainWindow::do_actionLinkedDocument, Qt::UniqueConnection);
}

void MainWindow::IniTreeView(QTreeView* view)
{
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setDragDropMode(QAbstractItemView::InternalMove);
    view->setDropIndicatorShown(true);
    view->setSortingEnabled(true);
    //    view->setColumnHidden(1, true);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setExpandsOnDoubleClick(true);
    view->expandAll();
    view->installEventFilter(this);
}

void MainWindow::do_actionAppend()
{
    auto widget = ui->tabWidget->currentWidget();
    if (!IsTreeView(widget))
        return;

    auto view = financial_.first;
    if (!HasSelection(view))
        return;

    auto index = view->currentIndex();
    if (!index.isValid())
        return;

    bool placeholder = index.siblingAtColumn(Tree::kPlaceholder).data().toBool();
    if (!placeholder)
        return;

    auto model = financial_.second;

    bool rule = index.siblingAtColumn(Tree::kRule).data().toBool();

    auto new_node = NodePool::Instance().Allocate();
    new_node->rule = rule;

    auto dialog = new InsertNode(new_node, view);
    if (dialog->exec() == QDialog::Rejected)
        return;

    if (model->insertRow(0, index, new_node)) {
        auto child_index = model->index(0, 0, index);
        view->setCurrentIndex(child_index);
    }
}

void MainWindow::do_actionJump()
{
    auto widget = ui->tabWidget->currentWidget();

    if (!IsTableView(widget))
        return;

    auto view = GetTableView(widget);
    if (!HasSelection(view))
        return;

    auto model = GetTableModel(view);
    if (!model)
        return;

    auto trans_index = view->currentIndex();
    if (!trans_index.isValid())
        return;

    auto transaction = model->GetTransaction(trans_index);
    int trans_id = transaction->id;

    auto transfer_id = transaction->transfer;
    if (transfer_id == 0)
        return;

    auto transfer_node = financial_.second->GetNode(transfer_id);
    if (!transfer_node)
        return;

    if (opened_table_view_.contains(transfer_id)) {
        SwitchTab(transfer_id);
    } else {
        TableInfo info(transfer_id, transfer_node->name, 2, transfer_node->rule, kFinancialTransactionTable);
        CreateTable(info, ui->tabWidget, financial_.second);
    }

    auto transfer_view = opened_table_view_.value(transfer_id);
    auto transfer_model = qobject_cast<TableModel*>(transfer_view->model());

    int trans_row = transfer_model->GetRow(trans_id);
    if (trans_row == -1)
        return;
    auto transfer_index = transfer_model->index(trans_row, 0, QModelIndex());
    transfer_view->setCurrentIndex(transfer_index);
}

void MainWindow::do_treeView_customContextMenuRequested(const QPoint& pos)
{
    Q_UNUSED(pos);

    auto menu = new QMenu(this);

    menu->addAction(ui->actionDelete);
    menu->addAction(ui->actionNew);
    menu->addAction(ui->actionAppend);

    menu->exec(QCursor::pos());
}

void MainWindow::do_actionEditNode()
{
    if (!IsTreeView(ui->tabWidget->currentWidget()))
        return;

    if (!HasSelection(financial_.first))
        return;

    auto index = financial_.first->currentIndex();
    if (!index.isValid())
        return;

    int node_id = index.siblingAtColumn(Tree::kID).data().toInt();
    auto node = financial_.second->GetNode(node_id);
    bool usage = financial_.second->UsageOfNode(node_id);

    auto dialog = new EditNode(node, usage, financial_.first);
    connect(dialog, &EditNode::SendUpdate, financial_.second, &TreeModel::ReceiveUpdate, Qt::UniqueConnection);
    dialog->exec();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(watched, event);

    auto key = static_cast<QKeyEvent*>(event)->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (watched->inherits("QTableView")) {
            InsertRowInTable(ui->tabWidget->currentWidget());
        }

        if (watched->inherits("QTreeView")) {
            auto view = qobject_cast<QTreeView*>(watched);
            auto index = view->currentIndex();
            do_treeView_doubleClicked(index);
        }

        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

template <typename T>
bool MainWindow::HasSelection(T* view)
{
    if (!view)
        return false;
    auto selection_model = view->selectionModel();
    return selection_model && selection_model->hasSelection();
}

void MainWindow::do_actionLinkedDocument()
{
    auto widget = ui->tabWidget->currentWidget();

    if (!IsTableView(widget))
        return;

    auto view = GetTableView(widget);
    if (!HasSelection(view))
        return;

    auto model = GetTableModel(view);
    if (!model)
        return;

    auto trans_index = view->currentIndex();
    if (!trans_index.isValid())
        return;

    auto transaction = model->GetTransaction(trans_index);
    auto dialog = new LinkedDocument(transaction, home_dir_, view);
    connect(dialog, &LinkedDocument::SendUpdate, &SignalManager::Instance(), &SignalManager::ReceiveUpdate, Qt::UniqueConnection);
    connect(dialog, &LinkedDocument::SendDocument, model, &TableModel::ReceiveDocument, Qt::UniqueConnection);
    dialog->exec();
}
