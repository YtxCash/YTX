#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QTableView>
#include <QTranslator>

#include "component/data.h"
#include "component/settings.h"
#include "component/using.h"
#include "database/mainwindowsqlite.h"
#include "table/model/tablemodel.h"
#include "ui_mainwindow.h"

using PDialog = QPointer<QDialog>;

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
    void RPrepAppendTriggered();
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

    void RUpdateSettings(CSectionRule& section_rule, const Interface& interface);
    void RUpdateName(const Node* node);

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
    inline bool IsTreeWidget(const QWidget* widget) { return widget->inherits("AbstractTreeWidget"); }
    inline bool IsTableWidget(const QWidget* widget) { return widget->inherits("TableWidget"); }
    inline TableModel* GetTableModel(QTableView* view) { return qobject_cast<TableModel*>(view->model()); }
    inline QTableView* GetQTableView(QWidget* widget) { return dynamic_cast<TableWidget*>(widget)->View(); }

private:
    void SetTabWidget();
    void SetConnect();
    void SetDateFormat();
    void SetHeader();
    void SetAction();
    void SetClearMenuAction();

    void SetFinanceData();
    void SetProductData();
    void SetStakeholderData();
    void SetTaskData();
    void SetSalesData();
    void SetPurchaseData();

    void CreateTable(SectionData* data, TreeModel* tree_model, CSectionRule* section_rule, TableHash* table_hash, int node_id);
    void CreateDelegate(QTableView* view, const TreeModel* tree_model, CSectionRule* section_rule, int node_id);
    void DelegateStakeholder(QTableView* view, CSectionRule* section_rule);
    void SetView(QTableView* view);
    void SetConnect(const QTableView* view, const TableModel* table, const TreeModel* tree, const SectionData* data);

    void CreateSection(Tree& tree, CString& name, SectionData* data, TableHash* table_hash, CSectionRule* section_rule);
    void SwitchSection(const Tab& last_tab);
    void SwitchDialog(QList<PDialog>* dialog_list, bool enable);
    void UpdateLastTab();

    void CreateDelegate(QTreeView* view, CInfo* info, CSectionRule* section_rule);
    void DelegateCommon(QTreeView* view, CInfo* info);
    void DelegateFinance(QTreeView* view, CInfo* info, CSectionRule* section_rule);
    void DelegateProduct(QTreeView* view, CInfo* info, CSectionRule* section_rule);
    void DelegateStakeholder(QTreeView* view, CSectionRule* section_rule);
    void DelegateOrder(QTreeView* view, CInfo* info, CSectionRule* section_rule);

    void SetView(QTreeView* view);
    void SetConnect(const QTreeView* view, const TreeWidget* widget, const TreeModel* model, const Sqlite* table_sql);

    void PrepInsertNode(QTreeView* view);
    void InsertNode(const QModelIndex& parent, int row);

    void AppendTrans(QWidget* widget);

    void SwitchTab(int node_id, int trans_id = 0);
    bool LockFile(CString& absolute_path, CString& complete_base_name);

    void DeleteTrans(QWidget* widget);
    void RemoveNode(QTreeView* view, TreeModel* model);
    void RemoveView(TreeModel* model, const QModelIndex& index, int node_id);
    void RemoveBranch(TreeModel* model, const QModelIndex& index, int node_id);

    void UpdateInterface(const Interface& interface);
    void UpdateTranslate();
    void UpdateRecent();

    void LoadAndInstallTranslator(CString& language);
    void ResizeColumn(QHeaderView* header, bool table_view = true);

    void SharedInterface(CString& dir_path);
    void ExclusiveInterface(CString& dir_path, CString& base_name);
    void ResourceFile();
    void Recent();

    void SaveTableWidget(const TableHash& table_hash, CString& section_name, CString& property);
    void RestoreTableWidget(SectionData* data, TreeModel* tree_model, CSectionRule* section_rule, TableHash* table_hash, CString& property);

    template <InheritQAbstractItemView T> bool HasSelection(const T* view) { return view && view->selectionModel() && view->selectionModel()->hasSelection(); }

    template <InheritQWidget T> void FreeWidget(T*& widget)
    {
        if (widget) {
            if (auto model = widget->Model()) {
                widget->SetModel(nullptr);
                delete model;
            }

            delete widget;
            widget = nullptr;
        }
    }

    template <InheritQWidget T> void SaveState(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto state { widget->saveState() };
        interface->setValue(QString("%1/%2").arg(section_name, property), state);
    }

    template <InheritQWidget T> void RestoreState(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto state { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };

        if (!state.isEmpty())
            widget->restoreState(state);
    }

    template <InheritQWidget T> void SaveGeometry(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto geometry { widget->saveGeometry() };
        interface->setValue(QString("%1/%2").arg(section_name, property), geometry);
    }

    template <InheritQWidget T> void RestoreGeometry(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto geometry { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };
        if (!geometry.isEmpty())
            widget->restoreGeometry(geometry);
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

    QStringList date_format_list_ {};

    Tree* section_tree_ {};
    TableHash* section_table_ {};
    QList<PDialog>* section_dialog_ {};
    SectionRule* section_rule_ {};
    SectionData* section_data_ {};

    Tree finance_tree_ {};
    TableHash finance_table_ {};
    QList<PDialog> finance_dialog_ {};
    SectionRule finance_rule_ {};
    SectionData finance_data_ {};

    Tree sales_tree_ {};
    TableHash sales_table_ {};
    QList<PDialog> sales_dialog_ {};
    SectionRule sales_rule_ {};
    SectionData sales_data_ {};

    Tree product_tree_ {};
    TableHash product_table_ {};
    QList<PDialog> product_dialog_ {};
    SectionRule product_rule_ {};
    SectionData product_data_ {};

    Tree stakeholder_tree_ {};
    TableHash stakeholder_table_ {};
    QList<PDialog> stakeholder_dialog_ {};
    SectionRule stakeholder_rule_ {};
    SectionData stakeholder_data_ {};

    Tree task_tree_ {};
    TableHash task_table_ {};
    QList<PDialog> task_dialog_ {};
    SectionRule task_rule_ {};
    SectionData task_data_ {};

    Tree purchase_tree_ {};
    TableHash purchase_table_ {};
    QList<PDialog> purchase_dialog_ {};
    SectionRule purchase_rule_ {};
    SectionData purchase_data_ {};
};
#endif // MAINWINDOW_H
