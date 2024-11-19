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

#ifndef TABLEMODELHELPER_H
#define TABLEMODELHELPER_H

#include "tablemodel.h"

class TableModelHelper final : public TableModel {
    Q_OBJECT
public:
    TableModelHelper(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);

public slots:
    // receive from TableModel
    void RAppendHelperTrans(const TransShadow* trans_shadow);
    void RRemoveHelperTrans(int helper_id, int trans_id);

    // receive from SignalStation
    void RAppendMultiHelperTransFPTS(int new_helper_id, const QList<int>& trans_id_list);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    bool IsHelper() const override { return true; }

protected:
    bool RemoveMultiTrans(const QList<int>& trans_id_list) override;
};

#endif // TABLEMODELHELPER_H
