#include "global/signalstation.h"

#include "table/model/tablemodelhelper.h"
#include "table/model/tablemodelstakeholder.h"

SignalStation& SignalStation::Instance()
{
    static SignalStation instance {};
    return instance;
}

void SignalStation::RegisterModel(Section section, int node_id, const TableModel* model)
{
    if (!model_hash_.contains(section))
        model_hash_[section] = QHash<int, const TableModel*>();

    model_hash_[section].insert(node_id, model);
}

void SignalStation::DeregisterModel(Section section, int node_id) { model_hash_[section].remove(node_id); }

void SignalStation::RAppendOneTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    int rhs_node_id { *trans_shadow->rhs_node };
    const auto* model { FindTableModel(section, rhs_node_id) };
    if (!model)
        return;

    connect(this, &SignalStation::SAppendOneTrans, model, &TableModel::RAppendOneTrans, Qt::SingleShotConnection);
    emit SAppendOneTrans(trans_shadow);
}

void SignalStation::RRemoveOneTrans(Section section, int node_id, int trans_id)
{
    const auto* model { FindTableModel(section, node_id) };
    if (!model)
        return;

    connect(this, &SignalStation::SRemoveOneTrans, model, &TableModel::RRemoveOneTrans, Qt::SingleShotConnection);
    emit SRemoveOneTrans(node_id, trans_id);
}

void SignalStation::RUpdateBalance(Section section, int node_id, int trans_id)
{
    const auto* model { FindTableModel(section, node_id) };
    if (!model)
        return;

    connect(this, &SignalStation::SUpdateBalance, model, &TableModel::RUpdateBalance, Qt::SingleShotConnection);
    emit SUpdateBalance(node_id, trans_id);
}

void SignalStation::RAppendHelperTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    const auto* model { FindTableModel(section, *trans_shadow->helper_node) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelHelper*>(model) };
    connect(this, &SignalStation::SAppendHelperTrans, cast_model, &TableModelHelper::RAppendHelperTrans, Qt::SingleShotConnection);
    emit SAppendHelperTrans(trans_shadow);
}

void SignalStation::RRemoveHelperTrans(Section section, int helper_id, int trans_id)
{
    const auto* model { FindTableModel(section, helper_id) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelHelper*>(model) };
    connect(this, &SignalStation::SRemoveHelperTrans, cast_model, &TableModelHelper::RRemoveHelperTrans, Qt::SingleShotConnection);
    emit SRemoveHelperTrans(helper_id, trans_id);
}

void SignalStation::RAppendPrice(Section section, TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    int lhs_node { *trans_shadow->lhs_node };
    const auto* model { FindTableModel(section, lhs_node) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelStakeholder*>(model) };
    connect(this, &SignalStation::SAppendPrice, cast_model, &TableModelStakeholder::RAppendPrice, Qt::SingleShotConnection);
    emit SAppendPrice(trans_shadow);
}

void SignalStation::RRule(Section section, int node_id, bool rule)
{
    const auto* model { FindTableModel(section, node_id) };
    if (!model)
        return;

    connect(this, &SignalStation::SRule, model, &TableModel::RRule, Qt::SingleShotConnection);
    emit SRule(node_id, rule);
}

void SignalStation::RMoveMultiHelperTransFPTS(Section section, int new_helper_id, const QList<int>& trans_id_list)
{
    const auto* model { FindTableModel(section, new_helper_id) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelHelper*>(model) };
    connect(this, &SignalStation::SAppendMultiHelperTransFPTS, cast_model, &TableModelHelper::RAppendMultiHelperTransFPTS, Qt::SingleShotConnection);
    emit SAppendMultiHelperTransFPTS(new_helper_id, trans_id_list);
}
