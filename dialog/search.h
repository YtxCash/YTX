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

#ifndef SEARCH_H
#define SEARCH_H

#include <QDialog>
#include <QTableView>

#include "component/settings.h"
#include "table/searchnodemodel.h"
#include "table/searchtransmodel.h"

namespace Ui {
class Search;
}

class Search final : public QDialog {
    Q_OBJECT

public:
    Search(CTreeModel* tree, CTreeModel* stakeholder_tree, CTreeModel* product_tree, CSettings* settings, Sqlite* sql, CInfo& info, QWidget* parent = nullptr);
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

    CSettings* settings_;
    CInfo& info_;
};

#endif // SEARCH_H
