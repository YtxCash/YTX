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
concept InheritQAbstractItemView = std::is_base_of_v<QAbstractItemView, T>;

template <typename T>
concept InheritQWidget = std::is_base_of_v<QWidget, T>;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(CString& dir_path, QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void ROpenFile(CString& file_path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void RInsertTriggered();
    void RRemoveTriggered();
    void RAppendNodeTriggered();
    void RJumpTriggered();
    void RAboutTriggered();
    void RPreferencesTriggered();
    void RSearchTriggered();
    void RClearMenuTriggered();
    void RNewTriggered();
    void ROpenTriggered();

    void RTreeLocation(int node_id);
    void RTableLocation(int trans_id, int lhs_node_id, int rhs_node_id);

    void REditNode();
    void REditDocument();

    void RUpdateSettings(CSettings& settings, CInterface& interface);
    void RUpdateName(int node_id, CString& name, bool branch);

    void RTabCloseRequested(int index);
    void RFreeView(int node_id);

    void RTreeViewCustomContextMenuRequested(const QPoint& pos);

    void RTabBarDoubleClicked(int index);
    void RTreeViewDoubleClicked(const QModelIndex& index);

    void RUpdateState();

    void on_rBtnFinance_toggled(bool checked);
    void on_rBtnSales_toggled(bool checked);
    void on_rBtnTask_toggled(bool checked);
    void on_rBtnStakeholder_toggled(bool checked);
    void on_rBtnProduct_toggled(bool checked);
    void on_rBtnPurchase_toggled(bool checked);

private:
    inline bool IsTreeWidget(const QWidget* widget) const { return widget && widget->inherits("TreeWidget"); }
    inline bool IsTableWidget(const QWidget* widget) const { return widget && widget->inherits("TableWidget"); }
    inline PTableModel GetTableModel(QWidget* widget) const
    {
        if (!widget)
            return nullptr;

        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        return static_cast<TableWidget*>(widget)->Model();
    }
    inline PQTableView GetQTableView(QWidget* widget) const
    {
        if (!widget)
            return nullptr;

        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        return static_cast<TableWidget*>(widget)->View();
    }

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
    void CreateTableOrder(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id, int party_id);
    void DelegateCommon(PQTableView table_view, PTreeModel tree_model, int node_id) const;
    void DelegateFinance(PQTableView table_view, CSettings* settings) const;
    void DelegateTask(PQTableView table_view, CSettings* settings) const;
    void DelegateProduct(PQTableView table_view, CSettings* settings) const;
    void DelegateStakeholder(PQTableView table_view, CSettings* settings) const;
    void DelegateOrder(PQTableView table_view, CSettings* settings) const;
    void SetView(PQTableView table_view) const;

    void TableConnectFPT(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, CData* data) const;
    void TableConnectOrder(PQTableView table_view, TableModelOrder* table_model, PTreeModel tree_model, TableWidgetOrder* widget) const;
    void TableConnectStakeholder(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, CData* data) const;

    void CreateSection(TreeWidget* tree_widget, TableHash& table_hash, CData& data, CSettings& settings, CString& name);
    void SwitchSection(CTab& last_tab) const;
    void UpdateLastTab() const;

    void SetDelegate(PQTreeView tree_view, CInfo& info, CSettings& settings) const;
    void DelegateCommon(PQTreeView tree_view, CInfo& info) const;
    void DelegateFinance(PQTreeView tree_view, CInfo& info, CSettings& settings) const;
    void DelegateTask(PQTreeView tree_view, CSettings& settings) const;
    void DelegateProduct(PQTreeView tree_view, CSettings& settings) const;
    void DelegateStakeholder(PQTreeView tree_view, CSettings& settings) const;
    void DelegateOrder(PQTreeView tree_view, CInfo& info, CSettings& settings) const;

    void SetView(PQTreeView tree_view) const;
    void TreeConnect(TreeWidget* tree_widget, const Sqlite* sql) const;

    void InsertNode(TreeWidget* tree_widget);
    void InsertNodeFunction(const QModelIndex& parent, int parent_id, int row);
    void InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row); // Finance Product Stakeholder Task
    void InsertNodeOrder(Node* node, const QModelIndex& parent, int row); // Purchase Sales

    void AppendTrans(TableWidget* table_widget);

    void EditNodeFPTS(const QModelIndex& index, int node_id); // Finance Product Stakeholder Task
    void SwitchTab(int node_id, int trans_id = 0) const;
    bool LockFile(CString& absolute_path, CString& complete_base_name) const;

    void RemoveTrans(TableWidget* table_widget);
    void RemoveNode(TreeWidget* tree_widget);
    void RemoveView(PTreeModel tree_model, const QModelIndex& index, int node_id);
    void RemoveBranch(PTreeModel tree_model, const QModelIndex& index, int node_id);

    void UpdateInterface(CInterface& interface);
    void UpdateTranslate() const;
    void UpdateRecent() const;

    void LoadAndInstallTranslator(CString& language);
    void ResizeColumn(QHeaderView* header, bool table_view = true) const;

    void SharedInterface(CString& dir_path);
    void ExclusiveInterface(CString& dir_path, CString& base_name);
    void ResourceFile() const;
    void Recent();

    void SaveTab(CTableHash& table_hash, CString& section_name, CString& property) const;
    void RestoreTab(PTreeModel tree_model, TableHash& table_hash, CData& data, CSettings& settings, CString& property);

    template <InheritQAbstractItemView T> bool HasSelection(QPointer<T> view) const
    {
        return view && view->selectionModel() && view->selectionModel()->hasSelection();
    }

    template <InheritQWidget T> void FreeWidget(T*& widget)
    {
        if (widget) {
            if (auto model = widget->Model())
                delete model;

            delete widget;
            widget = nullptr;
        }
    }

    template <InheritQWidget T> void SaveState(T* widget, QSettings* interface, CString& section_name, CString& property) const
    {
        auto state { widget->saveState() };
        interface->setValue(QString("%1/%2").arg(section_name, property), state);
    }

    template <InheritQWidget T> void RestoreState(T* widget, QSettings* interface, CString& section_name, CString& property) const
    {
        auto state { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };

        if (!state.isEmpty())
            widget->restoreState(state);
    }

    template <InheritQWidget T> void SaveGeometry(T* widget, QSettings* interface, CString& section_name, CString& property) const
    {
        auto geometry { widget->saveGeometry() };
        interface->setValue(QString("%1/%2").arg(section_name, property), geometry);
    }

    template <InheritQWidget T> void RestoreGeometry(T* widget, QSettings* interface, CString& section_name, CString& property) const
    {
        auto geometry { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };
        if (!geometry.isEmpty())
            widget->restoreGeometry(geometry);
    }

    template <typename Container> void SwitchDialog(Container* container, bool enable) const
    {
        if (container) {
            for (auto dialog : *container) {
                if (dialog) {
                    dialog->setVisible(enable);
                }
            }
        }
    }

private:
    Ui::MainWindow* ui {};
    MainwindowSqlite sql_ {};

    QStringList recent_list_ {};
    Section start_ {};

    QTranslator base_translator_ {};
    QTranslator cash_translator_ {};

    QString dir_path_ {};

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
