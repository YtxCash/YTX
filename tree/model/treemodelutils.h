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

    template <typename T> static const T& GetValue(CNodeHash& hash, int node_id, T Node::* member)
    {
        if (auto it = hash.constFind(node_id); it != hash.constEnd())
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
    static void SetParent(CNodeHash& hash, Node* root, Node* node, int parent_id);

    static Node* GetNodeByID(CNodeHash& hash, int node_id);
    static bool IsDescendant(Node* lhs, Node* rhs);
    static bool ChildrenEmpty(CNodeHash& hash, int node_id);
    static void SearchNodeFPTS(CNodeHash& hash, QList<const Node*>& node_list, const QList<int>& node_id_list);

    static void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare);
    static void UpdateComboModel(QStandardItemModel* model, const QVector<std::pair<QString, int>>& items);

    static QString GetPathFPTS(CStringHash& leaf, CStringHash& branch, int node_id);
    static QString ConstructPathFPTS(const Node* root, const Node* node, CString& separator);
    static void UpdatePathFPTS(StringHash& leaf, StringHash& branch, const Node* root, const Node* node, CString& separator);
    static void UpdateSeparatorFPTS(StringHash& leaf, StringHash& branch, CString& old_separator, CString& new_separator);
    static void PathPreferencesFPT(CNodeHash& hash, CStringHash& leaf, CStringHash& branch, QStandardItemModel* model);
    static void LeafPathRhsNodeFPT(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_node, Filter filter);
    static void LeafPathSpecificUnitPS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, Filter filter);
    static void LeafPathRemoveNodeFPTS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, int exclude_node);
    static void LeafPathHelperNodeFTS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_node, Filter filter);
    static bool HasChildrenFPTS(Node* node, CString& message);
    static bool IsBranchFPTS(Node* node, CString& message);
    static bool IsHelperFPTS(Node* node, CString& message);
    static bool IsOpenedFPTS(CTableHash& hash, int node_id, CString& message);
    static QStringList ChildrenNameFPTS(CNodeHash& hash, Node* root, int node_id, int exclude_child);

    static void UpdateBranchUnitF(const Node* root, Node* node);
    static void CopyNodeFPTS(CNodeHash& hash, Node* tmp_node, int node_id);
    static void UpdateAncestorValueFPT(QMutex& mutex, const Node* root, Node* node, double initial_diff, double final_diff);
    static void ShowTemporaryTooltipFPTS(CString& message, int duration);
    static QSet<int> ChildrenSetFPTS(CNodeHash& hash, int node_id);

    static bool IsInternalReferencedFPTS(Sqlite* sql, int node_id, CString& message);
    static bool IsHelperReferencedFPTS(Sqlite* sql, int node_id, CString& message);
    static bool IsExternalReferencedPS(Sqlite* sql, int node_id, CString& message);
};

#endif // TREEMODELUTILS_H
