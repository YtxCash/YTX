#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dialog/deletenode.h"
#include "table/tablemodel.h"
#include "tree/treemodel.h"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QTableView>
#include <QTreeView>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void do_actionNew();
    void do_actionDelete();
    void do_actionAppend();
    void do_actionJump();
    void do_actionEditNode();

    void do_releaseView(int index);
    void do_treeView_doubleClicked(const QModelIndex& index);
    void do_treeView_customContextMenuRequested(const QPoint& pos);

    void do_actionLinkedDocument();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void IniContainer(QTabWidget* container);
    void IniConnect();

    void CreateTable(const TableInfo& info, QTabWidget* container, TreeModel* tree_model);
    void CreateTableDelegate(QTableView* view, const QMultiMap<QString, int>& leaf_map, TreeModel* tree_model);
    void IniTableView(QTableView* view);
    void IniTableHeaderView(QTableView* view);
    void IniTableConnect(TableModel* table, TreeModel* tree);

    QPair<QTreeView*, TreeModel*> CreateTree(const TreeInfo& info);
    void CreateTreeDelegate(QTreeView* view);
    void IniTreeView(QTreeView* view);
    void IniTreeHeaderView(QTreeView* view);

    void InsertRowInTree(QTreeView* view, TreeModel* model);
    void InsertRowInTable(QWidget* widget);

    void SwitchTab(int node_id);

    void FreeView(QTableView* view);

    void DeleteFromTableView(QWidget* widget);
    void DeleteFromTreeView(QTreeView* view, TreeModel* model);
    void IniDeleteNodeConnect(DeleteNode* dialog, TreeModel* model);
    void ShowPlaceholderMsgBox(TreeModel* model, const QModelIndex& index);
    void DeleteNodeAndRelatedView(TreeModel* model, const QModelIndex& index, int node_id);

    QTableView* GetTableView(QWidget* widget);
    TableModel* GetTableModel(QTableView* view);

    template <typename T>
    bool HasSelection(T* view);

    bool IsTreeView(QWidget* widget);
    bool IsTableView(QWidget* widget);

    void RemoveOneLeaf(QMultiMap<QString, int>& leaf_map, int node_id);

private:
    Ui::MainWindow* ui {};

    QHash<int, QTableView*> opened_table_view_ {};

    QString date_format_ {};
    int decimal_ {};
    QString home_dir_ {};

    QPair<QTreeView*, TreeModel*> financial_ {};
    const QString kFinancialTable = "financial";
    const QString kFinancialPathTable = "financial_path";
    const QString kFinancialTransactionTable = "financial_transaction";
};
#endif // MAINWINDOW_H
