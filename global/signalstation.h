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

#ifndef SIGNALSTATION_H
#define SIGNALSTATION_H

#include "component/enumclass.h"
#include "table/model/tablemodel.h"
#include "table/trans.h"

class SignalStation final : public QObject {
    Q_OBJECT

public:
    static SignalStation& Instance();
    void RegisterModel(Section section, int node_id, const TableModel* model);
    void DeregisterModel(Section section, int node_id);

signals:
    // send to TableModel
    void SAppendOneTrans(const TransShadow* trans_shadow);
    void SRemoveOneTrans(int node_id, int trans_id);
    void SUpdateBalance(int node_id, int trans_id);
    void SRule(int node_id, bool rule);
    void SAppendPrice(TransShadow* trans_shadow);

    // send to TableModelHelper
    void SAppendHelperTrans(const TransShadow* trans_shadow);
    void SRemoveHelperTrans(int helper_id, int trans_id);
    void SAppendMultiHelperTransFPTS(int new_helper_id, const QList<int>& trans_id_list);

public slots:
    // receive from TableModel
    void RAppendOneTrans(Section section, const TransShadow* trans_shadow);
    void RRemoveOneTrans(Section section, int node_id, int trans_id);
    void RUpdateBalance(Section section, int node_id, int trans_id);

    void RAppendHelperTrans(Section section, const TransShadow* trans_shadow);
    void RRemoveHelperTrans(Section section, int helper_id, int trans_id);

    // receive from SqliteStakeholder
    void RAppendPrice(Section section, TransShadow* trans_shadow);

    // receive from TreeModel
    void RRule(Section section, int node_id, bool rule);

    // receive from sqlite
    void RMoveMultiHelperTransFPTS(Section section, int new_helper_id, const QList<int>& trans_id_list);

private:
    SignalStation() = default;
    ~SignalStation() { };

    SignalStation(const SignalStation&) = delete;
    SignalStation& operator=(const SignalStation&) = delete;
    SignalStation(SignalStation&&) = delete;
    SignalStation& operator=(SignalStation&&) = delete;

    const TableModel* FindTableModel(Section section, int node_id) const
    {
        auto it = model_hash_.constFind(section);
        if (it == model_hash_.constEnd())
            return nullptr;

        return it->value(node_id, nullptr);
    }

private:
    QHash<Section, QHash<int, const TableModel*>> model_hash_ {};
};

#endif // SIGNALSTATION_H
