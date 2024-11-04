#ifndef SEARCH_H
#define SEARCH_H

#include <QDialog>
#include <QTableView>

#include "component/settings.h"
#include "component/using.h"
#include "table/searchnodemodel.h"
#include "table/searchtransmodel.h"

namespace Ui {
class Search;
}

class Search final : public QDialog {
    Q_OBJECT

public:
    Search(CInfo& info, CTreeModel* tree, CTreeModel* stakeholder_tree, CTreeModel* product_tree, Sqlite* sql, CStringHash& rule_hash, CSettings& settings,
        QWidget* parent = nullptr);
    ~Search();

signals:
    void STreeLocation(int node_id);
    void STableLocation(int trans_id, int lhs_node_id, int rhs_node_id);

public slots:
    void RSearch();

private slots:
    void RDoubleClicked(const QModelIndex& index);

    void on_rBtnNode_toggled(bool checked);
    void on_rBtnTransaction_toggled(bool checked);

private:
    void IniDialog();
    void IniConnect();

    void TreeViewDelegate(QTableView* view, SearchNodeModel* model);
    void TableViewDelegate(QTableView* view, SearchTransModel* model);

    void IniView(QTableView* view);

    void ResizeTreeColumn(QHeaderView* header);
    void ResizeTableColumn(QHeaderView* header);

    void HideTreeColumn(QTableView* view, Section section);
    void HideTableColumn(QTableView* view, Section section);

private:
    Ui::Search* ui;

    SearchNodeModel* search_tree_ {};
    SearchTransModel* search_table_ {};
    Sqlite* sql_ {};
    CTreeModel* tree_ {};
    CTreeModel* stakeholder_tree_ {};
    CTreeModel* product_tree_ {};

    CSettings& settings_;
    CInfo& info_;
    CStringHash& rule_hash_;
};

#endif // SEARCH_H
