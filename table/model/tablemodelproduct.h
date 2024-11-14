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

#ifndef TABLEMODELPRODUCT_H
#define TABLEMODELPRODUCT_H

#include "tablemodel.h"

class TableModelProduct final : public TableModel {
    Q_OBJECT

public:
    TableModelProduct(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelProduct() override = default;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

protected:
    bool UpdateDebit(TransShadow* trans_shadow, double value) override;
    bool UpdateCredit(TransShadow* trans_shadow, double value) override;
    bool UpdateRatio(TransShadow* trans_shadow, double value) override;
};

#endif // TABLEMODELPRODUCT_H
