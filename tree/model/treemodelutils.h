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

#include <QStandardItemModel>

#include "component/using.h"
#include "tree/node.h"
#include "widget/tablewidget/tablewidget.h"

class TreeModelUtils {
public:
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

    static Node* GetNodeByID(CNodeHash& hash, int node_id);
    static bool IsDescendant(Node* lhs, Node* rhs);

    static void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare);
    static void UpdateComboModel(QStandardItemModel* model, const QVector<std::pair<QString, int>>& items);

    static QString ConstructPathFPTS(const Node* root, const Node* node, CString& separator);
    static void UpdatePathFPTS(StringHash& leaf, StringHash& branch, StringHash& helper, const Node* root, const Node* node, CString& separator);

    static void PathPreferencesFPT(CStringHash& leaf, CStringHash& branch, QStandardItemModel* model);
    static void LeafPathRhsNodeFPT(CStringHash& leaf, QStandardItemModel* model);
    static void LeafPathSpecificUnitPS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, Filter filter);
    static void LeafPathSpecificUnitPS(const QSet<int>& node_id, CStringHash& leaf, QStandardItemModel* model);
    static void LeafPathRemoveNodeFPTS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, int exclude_node);
    static void HelperPathFPTS(CStringHash& helper, QStandardItemModel* model, int specific_node, Filter filter);

    static void AddItemToModel(QStandardItemModel* model, CString& path, int node_id, bool should_sort = true);
    static void RemoveItemFromModel(QStandardItemModel* model, int node_id);
    static void UpdateModel(CStringHash& leaf, QStandardItemModel* leaf_model, CStringHash& helper, QStandardItemModel* helper_model, const Node* node);
    static void UpdateUnitModel(CStringHash& leaf, QStandardItemModel* unit_model, const Node* node, int specific_unit, Filter filter);
    static void UpdatePathSeparatorFPTS(CString& old_separator, CString& new_separator, StringHash& source_path);
    static void UpdateModelSeparatorFPTS(QStandardItemModel* model, CStringHash& source_path);

    static bool HasChildrenFPTS(Node* node, CString& message);
    static bool IsBranchFPTS(Node* node, CString& message);
    static bool IsHelperFPTS(Node* node, CString& message);
    static bool IsOpenedFPTS(CTableHash& hash, int node_id, CString& message);

    static void UpdateBranchUnitF(const Node* root, Node* node);
    static void UpdateAncestorValueFPT(QMutex& mutex, const Node* root, Node* node, double initial_diff, double final_diff);
    static void ShowTemporaryTooltipFPTS(CString& message, int duration);

    static bool IsInternalReferencedFPTS(Sqlite* sql, int node_id, CString& message);
    static bool IsHelperReferencedFPTS(Sqlite* sql, int node_id, CString& message);
    static bool IsExternalReferencedPS(Sqlite* sql, int node_id, CString& message);

private:
    static void UpdateModelFunction(QStandardItemModel* model, const QSet<int>& update_range, CStringHash& source_path);
};

#endif // TREEMODELUTILS_H
