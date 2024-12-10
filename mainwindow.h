/*
 * Copyright (C) 2023 YtxCash
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QFileInfo>
#include <QLockFile>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QTableView>
#include <QTranslator>

#include "component/settings.h"
#include "component/using.h"
#include "database/mainwindowsqlite.h"
#include "table/model/tablemodel.h"
#include "table/model/tablemodelorder.h"
#include "tree/model/treemodel.h"
#include "ui_mainwindow.h"
#include "widget/tablewidget/tablewidgetorder.h"
#include "widget/treewidget/treewidget.h"

struct Data {
    Tab tab {};
    Info info {};
    Sqlite* sql {};
};

using PDialog = QPointer<QDialog>;
using CData = const Data;

template <typename T>
concept TableWidgetLike = std::is_base_of_v<QWidget, T> && requires(T t) {
    { t.Model() } -> std::convertible_to<QPointer<TableModel>>;
    { t.View() } -> std::convertible_to<QPointer<QTableView>>;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(CString& config_dir, QWidget* parent = nullptr);
    ~MainWindow();

    bool OpenFile(CString& file_path);

public slots:
    void on_actionInsertNode_triggered();
    void on_actionAppendTrans_triggered();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void on_actionRemove_triggered();
    void on_actionAppendNode_triggered();
    void on_actionEditNode_triggered();
    void on_actionJump_triggered();
    void on_actionSupportJump_triggered();
    void on_actionAbout_triggered();
    void on_actionPreferences_triggered();
    void on_actionSearch_triggered();
    void on_actionClearMenu_triggered();
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabBarDoubleClicked(int index);
    void on_tabWidget_tabCloseRequested(int index);

    void on_rBtnFinance_toggled(bool checked);
    void on_rBtnSales_toggled(bool checked);
    void on_rBtnTask_toggled(bool checked);
    void on_rBtnStakeholder_toggled(bool checked);
    void on_rBtnProduct_toggled(bool checked);
    void on_rBtnPurchase_toggled(bool checked);

    void RNodeLocation(int node_id);
    void RTransLocation(int trans_id, int lhs_node_id, int rhs_node_id);

    void RUpdateSettings(CSettings& settings, CInterface& interface);
    void RUpdateParty(int node_id, int party_id);
    void RUpdateName(int node_id, CString& name, bool branch);
    void RUpdateState();

    void RFreeView(int node_id);
    void REditTransDocument();
    void REditNodeDocument();

    void RTreeViewCustomContextMenuRequested(const QPoint& pos);
    void RTreeViewDoubleClicked(const QModelIndex& index);

private:
    void SetHeader();
    void SetTabWidget();
    void SetClearMenuAction();

    void SetConnect() const;
    void SetAction() const;

    void SetFinanceData();
    void SetProductData();
    void SetStakeholderData();
    void SetTaskData();
    void SetSalesData();
    void SetPurchaseData();

    void CreateTableFPTS(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id);
    void CreateTableSupport(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id);
    void CreateTableOrder(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id, int party_id);
    void DelegateFPTS(PQTableView table_view, PTreeModel tree_model, CSettings* settings) const;
    void DelegateFPT(PQTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const;
    void DelegateStakeholder(PQTableView table_view) const;
    void DelegateOrder(PQTableView table_view, CSettings* settings) const;
    void SetView(PQTableView table_view) const;

    void SetSupportView(PQTableView table_view) const;
    void DelegateSupport(PQTableView table_view, PTreeModel tree_model, CSettings* settings) const;

    void TableConnectFPT(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, CData* data) const;
    void TableConnectOrder(PQTableView table_view, TableModelOrder* table_model, PTreeModel tree_model, TableWidgetOrder* widget) const;
    void TableConnectStakeholder(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, CData* data) const;

    void CreateSection(TreeWidget* tree_widget, TableHash& table_hash, CData& data, CSettings& settings, CString& name);
    void SwitchSection(CTab& last_tab) const;
    void UpdateLastTab() const;

    void SetDelegate(PQTreeView tree_view, CInfo& info, CSettings& settings) const;
    void DelegateFPTSO(PQTreeView tree_view, CInfo& info) const;
    void DelegateFinance(PQTreeView tree_view, CInfo& info, CSettings& settings) const;
    void DelegateTask(PQTreeView tree_view, CSettings& settings) const;
    void DelegateProduct(PQTreeView tree_view, CSettings& settings) const;
    void DelegateStakeholder(PQTreeView tree_view, CSettings& settings) const;
    void DelegateOrder(PQTreeView tree_view, CInfo& info, CSettings& settings) const;

    void SetView(PQTreeView tree_view) const;
    void TreeConnect(TreeWidget* tree_widget, const Sqlite* sql) const;

    void InsertNodeFunction(const QModelIndex& parent, int parent_id, int row);
    void InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row); // Finance Product Stakeholder Task
    void InsertNodeOrder(Node* node, const QModelIndex& parent, int row); // Purchase Sales

    template <TableWidgetLike T> void AppendTrans(T* widget);

    void EditNodeFPTS(const QModelIndex& index, int node_id); // Finance Product Stakeholder Task
    void SwitchTab(int node_id, int trans_id = 0) const;

    void RemoveTrans(TableWidget* table_widget);
    void RemoveNode(TreeWidget* tree_widget);
    void RemoveView(PTreeModel tree_model, const QModelIndex& index, int node_id, int node_type);
    void RemoveBranch(PTreeModel tree_model, const QModelIndex& index, int node_id);

    void UpdateInterface(CInterface& interface);
    void UpdateTranslate() const;
    void UpdateRecent() const;
    void UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const;

    void LoadAndInstallTranslator(CString& language);
    void ResizeColumn(QHeaderView* header, bool table_view = true) const;

    void SharedInterface(CString& dir_path);
    void ExclusiveInterface(CString& dir_path, CString& base_name);
    void ResourceFile() const;
    void Recent();
    bool LockFile(const QFileInfo file_info);

    void SaveTab(CTableHash& table_hash, CString& section_name, CString& property) const;
    void RestoreTab(PTreeModel tree_model, TableHash& table_hash, CData& data, CSettings& settings, CString& property);
    void EnableAction(bool enable);

    QStandardItemModel* CreateModelFromList(QStringList& list, QObject* parent = nullptr);

private:
    Ui::MainWindow* ui {};
    MainwindowSqlite sql_ {};

    QStringList recent_list_ {};
    Section start_ {};

    QTranslator qt_translator_ {};
    QTranslator ytx_translator_ {};

    QString config_dir_ {};
    std::unique_ptr<QLockFile> lock_file_;

    QSettings* shared_interface_ {};
    QSettings* exclusive_interface_ {};

    Interface interface_ {};

    TreeWidget* tree_widget_ {};
    TableHash* table_hash_ {};
    QList<PDialog>* dialog_list_ {};
    QHash<int, PDialog>* dialog_hash_ {};
    Settings* settings_ {};
    Data* data_ {};

    TreeWidget* finance_tree_ {};
    TableHash finance_table_hash_ {};
    QList<PDialog> finance_dialog_list_ {};
    QHash<int, PDialog> finance_dialog_hash_ {};
    Settings finance_settings_ {};
    Data finance_data_ {};

    TreeWidget* product_tree_ {};
    TableHash product_table_hash_ {};
    QList<PDialog> product_dialog_list_ {};
    QHash<int, PDialog> product_dialog_hash_ {};
    Settings product_settings_ {};
    Data product_data_ {};

    TreeWidget* task_tree_ {};
    TableHash task_table_hash_ {};
    QList<PDialog> task_dialog_list_ {};
    QHash<int, PDialog> task_dialog_hash_ {};
    Settings task_settings_ {};
    Data task_data_ {};

    TreeWidget* stakeholder_tree_ {};
    TableHash stakeholder_table_hash_ {};
    QList<PDialog> stakeholder_dialog_list_ {};
    QHash<int, PDialog> stakeholder_dialog_hash_ {};
    Settings stakeholder_settings_ {};
    Data stakeholder_data_ {};

    TreeWidget* sales_tree_ {};
    TableHash sales_table_hash_ {};
    QList<PDialog> sales_dialog_list_ {};
    QHash<int, PDialog> sales_dialog_hash_ {};
    Settings sales_settings_ {};
    Data sales_data_ {};

    TreeWidget* purchase_tree_ {};
    TableHash purchase_table_hash_ {};
    QList<PDialog> purchase_dialog_list_ {};
    QHash<int, PDialog> purchase_dialog_hash_ {};
    Settings purchase_settings_ {};
    Data purchase_data_ {};
};
#endif // MAINWINDOW_H
