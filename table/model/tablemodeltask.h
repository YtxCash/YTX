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

#ifndef TABLEMODELTASK_H
#define TABLEMODELTASK_H

#include "tablemodel.h"

class TableModelTask final : public TableModel {
    Q_OBJECT

public:
    TableModelTask(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelTask() override = default;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    bool UpdateDebit(TransShadow* trans_shadow, double value) override;
    bool UpdateCredit(TransShadow* trans_shadow, double value) override;
    bool UpdateRatio(TransShadow* trans_shadow, double value) override;
};

#endif // TABLEMODELTASK_H
