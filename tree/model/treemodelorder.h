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

#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include <QDate>

#include "database/sqlite/sqliteorder.h"
#include "tree/model/treemodel.h"
#include "treemodelhelper.h"

class TreeModelOrder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, QObject* parent = nullptr);
    ~TreeModelOrder() override;

signals:
    void SUpdateData(int node_id, TreeEnumOrder column, const QVariant& value);

public slots:
    void RUpdateLeafValueTO(int node_id, double diff, CString& node_field) override; // first
    void RUpdateLeafValueFPTO(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff) override;

    void RUpdateStakeholderSO(int old_node_id, int new_node_id) override;
    void RUpdateFinished(int node_id, bool checked);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return info_.tree_header.size();
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        return TreeModelHelper::headerData(info_, section, orientation, role);
    }

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;

    void UpdateTree(const QDate& start_date, const QDate& end_date);
    void SetParent(Node* node, int parent_id) const override;
    void SetNodeShadowOrder(NodeShadow* node_shadow, int node_id) const override;
    void SetNodeShadowOrder(NodeShadow* node_shadow, Node* node) const override;
    QModelIndex GetIndex(int node_id) const override;
    bool Contains(int node_id) const override { return node_hash_.contains(node_id); }
    bool ChildrenEmpty(int node_id) const override;
    int Unit(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::unit); }
    QString Name(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::name); }
    QString GetPath(int node_id) const override;
    bool Rule(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::rule); }
    void UpdateDefaultUnit(int default_unit) override { root_->unit = default_unit; }
    void RetriveNodeO(int node_id) override;

protected:
    bool UpdateRuleFPTO(Node* node, bool value) override; // charge = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateName(Node* node, CString& value) override;
    Node* GetNodeByIndex(const QModelIndex& index) const override;
    void ConstructTree() override;

private:
    bool UpdateFinished(Node* node, bool value);
    void UpdateAncestorValueOrder(
        Node* node, double first_diff, double second_diff = 0.0, double amount_diff = 0.0, double discount_diff = 0.0, double settled_diff = 0.0);

private:
    SqliteOrder* sql_ {};
    Node* root_ {};

    NodeHash node_hash_ {};

    CInfo& info_;
};

#endif // TREEMODELORDER_H
