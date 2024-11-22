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

#ifndef TREEMODELFINANCE_H
#define TREEMODELFINANCE_H

#include "tree/model/treemodel.h"

class TreeModelFinance final : public TreeModel {
    Q_OBJECT

public:
    TreeModelFinance(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelFinance() override;

public slots:
    void RUpdateLeafValue(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff,
        double settled_diff = 0.0) override;
    void RUpdateMultiLeafTotal(const QList<int>& node_list) override;

public:
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    void UpdateNodeFPTS(const Node* tmp_node) override;
    void UpdateDefaultUnit(int default_unit) override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

protected:
    void ConstructTree() override;
    bool UpdateUnit(Node* node, int value) override;
};

#endif // TREEMODELFINANCE_H
