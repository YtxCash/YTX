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

#ifndef TABLEMODELORDER_H
#define TABLEMODELORDER_H

#include "database/sqlite/sqlitestakeholder.h"
#include "tablemodel.h"
#include "tree/model/treemodel.h"
#include "tree/model/treemodelproduct.h"

class TableModelOrder final : public TableModel {
    Q_OBJECT

public:
    TableModelOrder(Sqlite* sql, bool rule, int node_id, CInfo& info, const NodeShadow* node_shadow, CTreeModel* product_tree, Sqlite* sqlite_stakeholder,
        QObject* parent = nullptr);
    ~TableModelOrder() override = default;

public slots:
    void RUpdateNodeID(int node_id);
    void RUpdateLocked(int node_id, bool checked);
    void RUpdateParty();

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

    int GetNodeRow(int node_id) const override;

private:
    template <typename T>
    bool UpdateField(TransShadow* trans_shadow, const T& value, CString& field, T* TransShadow::* member, const std::function<void()>& action = {}) const
    {
        if (trans_shadow == nullptr || trans_shadow->*member == nullptr || *(trans_shadow->*member) == value)
            return false;

        *(trans_shadow->*member) = value;

        if (*trans_shadow->node_id == 0 || *trans_shadow->lhs_node == 0)
            return false;

        try {
            sql_->UpdateField(info_.transaction, value, field, *trans_shadow->id);
            if (action)
                action();
        } catch (const std::exception& e) {
            qWarning() << "Failed in UpdateField" << e.what();
            return false;
        }

        return true;
    }

    bool UpdateInsideProduct(TransShadow* trans_shadow, int value) const;
    bool UpdateOutsideProduct(TransShadow* trans_shadow, int value) const;

    bool UpdateUnitPrice(TransShadow* trans_shadow, double value);
    bool UpdateDiscountPrice(TransShadow* trans_shadow, double value);
    bool UpdateSecond(TransShadow* trans_shadow, double value);

    void SearchPrice(TransShadow* trans_shadow, int product_id, bool is_inside) const;

private:
    const TreeModelProduct* product_tree_ {};
    SqliteStakeholder* sqlite_stakeholder_ {};
    QHash<int, double> update_price_ {}; // inside_product_id, exclusive_price
    const NodeShadow* node_shadow_ {};
};

#endif // TABLEMODELORDER_H
