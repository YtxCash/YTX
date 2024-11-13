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

#ifndef TREEMODELPRODUCT_H
#define TREEMODELPRODUCT_H

#include "tree/model/treemodel.h"
#include "treemodelutils.h"

class TreeModelProduct final : public TreeModel {
    Q_OBJECT

public:
    TreeModelProduct(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelProduct() override;

public slots:
    void RUpdateLeafValueFPTO(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff,
        double settled_diff = 0.0) override;
    void RUpdateMultiLeafTotalFPT(const QList<int>& node_list) override;

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
        return TreeModelUtils::headerData(info_, section, orientation, role);
    }

    void UpdateNodeFPTS(const Node* tmp_node) override;
    void UpdateSeparatorFPTS(CString& old_separator, CString& new_separator) override;
    void CopyNodeFPTS(Node* tmp_node, int node_id) const override;
    void SetParent(Node* node, int parent_id) const override;
    QStringList ChildrenNameFPTS(int node_id, int exclude_child) const override;
    QString GetPath(int node_id) const override;
    void LeafPathBranchPathFPT(QStandardItemModel* combo_model) const override;
    void LeafPathExcludeIDFPT(QStandardItemModel* combo_model, int exclude_id) const override;
    void LeafPathSpecificUnitPS(QStandardItemModel* combo_model, int unit, UnitFilterMode unit_filter_mode) const override;
    void LeafPathSpecificUnitExcludeIDFPTS(QStandardItemModel* combo_model, int unit, int exclude_id) const override;
    QModelIndex GetIndex(int node_id) const override;
    bool Contains(int node_id) const override { return node_hash_.contains(node_id); }
    bool ChildrenEmpty(int node_id) const override;
    double InitialTotalFPT(int node_id) const override { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::initial_total); }
    double FinalTotalFPT(int node_id) const override { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::final_total); }
    int Unit(int node_id) const override { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::unit); }
    QString Name(int node_id) const override { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::name); }
    bool BranchFPTS(int node_id) const override { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::branch); }
    bool Rule(int node_id) const override { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::rule); }
    void SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    void UpdateDefaultUnit(int default_unit) override { root_->unit = default_unit; }
    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;

    const QString& Color(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::color); }
    double First(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::first); }
    QSet<int> ChildrenSetFPTS(int node_id) const override;

protected:
    bool IsReferencedFPTS(int node_id, CString& message) const override;
    bool UpdateUnit(Node* node, int value) override;
    Node* GetNodeByIndex(const QModelIndex& index) const override;
    bool UpdateBranchFPTS(Node* node, bool value) override;
    bool UpdateName(Node* node, CString& value) override;
    bool UpdateRuleFPTO(Node* node, bool value) override;
    void ConstructTree() override;

private:
    Sqlite* sql_ {};
    Node* root_ {};
    QMutex mutex_ {};

    NodeHash node_hash_ {};
    StringHash leaf_path_ {};
    StringHash branch_path_ {};

    CInfo& info_;
    CTableHash& table_hash_;
    CString& separator_;
};

#endif // TREEMODELPRODUCT_H
