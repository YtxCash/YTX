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

#ifndef TREEMODELUTILS_H
#define TREEMODELUTILS_H

#include <QModelIndex>
#include <QStandardItemModel>
#include <QVariant>

#include "component/info.h"
#include "component/using.h"
#include "tree/node.h"
#include "widget/tablewidget/tablewidget.h"

class TreeModelUtils {
public:
    static QVariant headerData(const Info& info, int section, Qt::Orientation orientation, int role)
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return info.tree_header.at(section);
        }
        return QVariant();
    }

    static Node* GetNodeByIndex(Node* root, const QModelIndex& index)
    {
        if (index.isValid() && index.internalPointer())
            return static_cast<Node*>(index.internalPointer());

        return root;
    }

    template <typename T> static bool UpdateField(Sqlite* sql, Node* node, CString& table, const T& value, CString& field, T Node::* member)
    {
        if constexpr (std::is_floating_point_v<T>) {
            if (std::abs(node->*member - value) < TOLERANCE)
                return false;
        } else {
            if (node->*member == value)
                return false;
        }

        node->*member = value;
        sql->UpdateField(table, value, field, node->id);
        return true;
    }

    template <typename T> static const T& GetValue(CNodeHash& node_hash, int node_id, T Node::* member)
    {
        if (auto it = node_hash.constFind(node_id); it != node_hash.constEnd())
            return it.value()->*member;

        // If the node_id does not exist, return a static empty object to ensure a safe default value
        // Examples:
        // double InitialTotal(int node_id) const { return GetValue(node_id, &Node::initial_total); }
        // double FinalTotal(int node_id) const { return GetValue(node_id, &Node::final_total); }
        // Note: In the SetStatus() function of TreeWidget,
        // a node_id of 0 may be passed, so empty{} is needed to prevent illegal access
        static const T empty {};
        return empty;
    }

    static void InitializeRoot(Node*& root, int default_unit);
    static void SetParent(CNodeHash& node_hash, Node* root, Node* node, int parent_id);

    static Node* GetNodeByID(CNodeHash& node_hash, int node_id);
    static bool IsDescendant(Node* lhs, Node* rhs);
    static bool ChildrenEmpty(CNodeHash& node_hash, int node_id);
    static void SearchNodeFPTS(CNodeHash& node_hash, QList<const Node*>& node_list, const QList<int>& node_id_list);

    static void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare);
    static void UpdateComboModel(QStandardItemModel* combo_model, const QVector<std::pair<QString, int>>& items);

    static QString GetPathFPTS(CStringHash& leaf_path, CStringHash& branch_path, int node_id);
    static QString ConstructPathFPTS(const Node* root, const Node* node, CString& separator);
    static void UpdatePathFPTS(StringHash& leaf_path, StringHash& branch_path, const Node* root, const Node* node, CString& separator);
    static void UpdateSeparatorFPTS(StringHash& leaf_path, StringHash& branch_path, CString& old_separator, CString& new_separator);
    static void LeafPathBranchPathFPT(CStringHash& leaf_path, CStringHash& branch_path, QStandardItemModel* combo_model);
    static void LeafPathExcludeIDFPT(CStringHash& leaf_path, QStandardItemModel* combo_model, int exclude_id);
    static void LeafPathSpecificUnitPS(
        CNodeHash& node_hash, CStringHash& leaf_path, QStandardItemModel* combo_model, int unit, UnitFilterMode unit_filter_mode);
    static void LeafPathSpecificUnitExcludeIDFPTS(CNodeHash& node_hash, CStringHash& leaf_path, QStandardItemModel* combo_model, int unit, int exclude_id);
    static bool HasChildrenFPTS(Node* node, CString& message);
    static bool IsBranchFTS(Node* node, CString& message);
    static bool IsHelperFTS(Node* node, CString& message);
    static bool IsOpenedFPTS(CTableHash& table_hash, int node_id, CString& message);
    static QStringList ChildrenNameFPTS(CNodeHash& node_hash, Node* root, int node_id, int exclude_child);

    static void UpdateBranchUnitF(const Node* root, Node* node);
    static void CopyNodeFPTS(CNodeHash& node_hash, Node* tmp_node, int node_id);
    static void UpdateAncestorValueFPT(QMutex& mutex, const Node* root, Node* node, double initial_diff, double final_diff);
    static void ShowTemporaryTooltipFPTS(CString& message, int duration);
    static QSet<int> ChildrenSetFPTS(CNodeHash& node_hash, int node_id);

    static bool IsInternalReferencedFPTS(Sqlite* sql, int node_id, CString& message);
    static bool IsHelperReferencedFTS(Sqlite* sql, int node_id, CString& message);
    static bool IsExternalReferencedPS(Sqlite* sql, int node_id, CString& message);
};

#endif // TREEMODELUTILS_H
