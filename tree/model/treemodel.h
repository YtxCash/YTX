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

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>
#include <QStandardItemModel>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/using.h"
#include "tree/node.h"

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = nullptr);
    virtual ~TreeModel() = default;

protected:
    TreeModel() = delete;
    TreeModel(const TreeModel&) = delete;
    TreeModel& operator=(const TreeModel&) = delete;
    TreeModel(TreeModel&&) = delete;
    TreeModel& operator=(TreeModel&&) = delete;

signals:
    // send to SignalStation
    void SRule(Section seciton, int node_id, bool rule);

    // send to its view
    void SResizeColumnToContents(int column);

    // send to Search dialog
    void SSearch();

    // send to Mainwindow
    void SUpdateName(int node_id, CString& name, bool branch);
    void SUpdateDSpinBox();

    // send to SpecificUnit delegate, TableCombo delegate, EditNodeOrder and TableWidgetOrder
    void SUpdateComboModel();

public slots:
    // receive from Sqlite
    void RRemoveNode(int node_id);
    virtual void RUpdateMultiLeafTotalFPT(const QList<int>& /*node_list*/) { }

    // receive from  TableModel
    void RSearch() { emit SSearch(); }
    virtual void RUpdateLeafValueTO(int node_id, double diff, CString& node_field)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(diff);
        Q_UNUSED(node_field);
    }
    virtual void RUpdateLeafValueFPTO(
        int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double settled_diff)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(initial_debit_diff);
        Q_UNUSED(initial_credit_diff);
        Q_UNUSED(final_debit_diff);
        Q_UNUSED(final_credit_diff);
        Q_UNUSED(settled_diff);
    }

    virtual void RUpdateStakeholderSO(int old_node_id, int new_node_id)
    {
        Q_UNUSED(old_node_id);
        Q_UNUSED(new_node_id);
    };

public:
    // Qt's
    // Default implementations
    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }
    QStringList mimeTypes() const override { return QStringList { NODE_ID }; }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& /*parent*/) const override
    {
        return data && data->hasFormat(NODE_ID) && action != Qt::IgnoreAction;
    }

    // Ytx's
    // Default implementations
    virtual double InitialTotalFPT(int node_id) const
    {
        Q_UNUSED(node_id);
        return {};
    }
    virtual double FinalTotalFPT(int node_id) const
    {
        Q_UNUSED(node_id);
        return {};
    }
    virtual QStringList ChildrenNameFPTS(int node_id, int exclude_child) const
    {
        Q_UNUSED(node_id);
        Q_UNUSED(exclude_child);
        return {};
    }
    virtual bool BranchFPTS(int node_id) const
    {
        Q_UNUSED(node_id);
        return {};
    }
    virtual void CopyNodeFPTS(Node* tmp_node, int node_id) const
    {
        Q_UNUSED(tmp_node);
        Q_UNUSED(node_id);
    }
    virtual void PathPreferencesFPT(QStandardItemModel* model) const { Q_UNUSED(model); }
    virtual void LeafPathRhsNodeFPT(QStandardItemModel* model, int specific_node, Filter filter) const
    {
        Q_UNUSED(model);
        Q_UNUSED(specific_node);
        Q_UNUSED(filter);
    }
    virtual void LeafPathRemoveNodeFPTS(QStandardItemModel* model, int specific_unit, int exclude_node) const
    {
        Q_UNUSED(model);
        Q_UNUSED(specific_unit);
        Q_UNUSED(exclude_node);
    }
    virtual void LeafPathHelperNodeFTS(QStandardItemModel* model, int specific_node, Filter filter) const
    {
        Q_UNUSED(model);
        Q_UNUSED(specific_node);
        Q_UNUSED(filter);
    }
    virtual void LeafPathSpecificUnitPS(QStandardItemModel* model, int specific_unit, Filter filter) const
    {
        Q_UNUSED(model);
        Q_UNUSED(specific_unit);
        Q_UNUSED(filter);
    }
    virtual void SetNodeShadowOrder(NodeShadow* node_shadow, int node_id) const
    {
        Q_UNUSED(node_shadow);
        Q_UNUSED(node_id);
    }
    virtual void SetNodeShadowOrder(NodeShadow* node_shadow, Node* node) const
    {
        Q_UNUSED(node_shadow);
        Q_UNUSED(node);
    }
    virtual void UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
    {
        Q_UNUSED(old_separator);
        Q_UNUSED(new_separator);
    }
    virtual void SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const
    {
        Q_UNUSED(node_list);
        Q_UNUSED(node_id_list);
    }

    virtual void UpdateNodeFPTS(const Node* tmp_node) { Q_UNUSED(tmp_node); }
    virtual void RetriveNodeO(int node_id) { Q_UNUSED(node_id); };

    virtual QSet<int> ChildrenSetFPTS(int node_id) const
    {
        Q_UNUSED(node_id);
        return {};
    }
    virtual bool IsHelperFPTS(int node_id)
    {
        Q_UNUSED(node_id);
        return {};
    }

    // Core pure virtual functions
    virtual void SetParent(Node* node, int parent_id) const = 0;
    virtual void UpdateDefaultUnit(int default_unit) = 0;

    virtual bool ChildrenEmpty(int node_id) const = 0;
    virtual bool Contains(int node_id) const = 0;
    virtual bool InsertNode(int row, const QModelIndex& parent, Node* node) = 0;
    virtual bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) = 0;
    virtual bool Rule(int node_id) const = 0;

    virtual QModelIndex GetIndex(int node_id) const = 0;
    virtual QString Name(int node_id) const = 0;
    virtual QString GetPath(int node_id) const = 0;
    virtual int Unit(int node_id) const = 0;

protected:
    // Core pure virtual functions
    virtual Node* GetNodeByIndex(const QModelIndex& index) const = 0;
    virtual bool UpdateName(Node* node, CString& value) = 0;
    virtual bool UpdateUnit(Node* node, int value) = 0;
    virtual void ConstructTree() = 0;

    // Default implementations
    virtual bool UpdateBranchFPTS(Node* node, bool value)
    {
        Q_UNUSED(node);
        Q_UNUSED(value);
        return {};
    }

    virtual bool UpdateHelperFPTS(Node* node, bool value)
    {
        Q_UNUSED(node);
        Q_UNUSED(value);
        return {};
    }

    virtual bool UpdateRuleFPTO(Node* node, bool value)
    {
        Q_UNUSED(node);
        Q_UNUSED(value);
        return {};
    }
};

using PTreeModel = QPointer<TreeModel>;
using CTreeModel = const TreeModel;

#endif // TREEMODEL_H
