#include "global/signalstation.h"

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
    auto section_model_hash { model_hash_.value(section) };
    int rhs_node_id { *trans_shadow->rhs_node };

    auto model { section_model_hash.value(rhs_node_id, nullptr) };
    if (!model)
        return;

    connect(this, &SignalStation::SAppendOneTrans, model, &TableModel::RAppendOneTrans, Qt::SingleShotConnection);
    emit SAppendOneTrans(trans_shadow);
}

void SignalStation::RRemoveOneTrans(Section section, int node_id, int trans_id)
{
    auto section_model_hash { model_hash_.value(section) };

    auto model { section_model_hash.value(node_id, nullptr) };
    if (!model)
        return;

    connect(this, &SignalStation::SRemoveOneTrans, model, &TableModel::RRemoveOneTrans, Qt::SingleShotConnection);
    emit SRemoveOneTrans(node_id, trans_id);
}

void SignalStation::RUpdateBalance(Section section, int node_id, int trans_id)
{
    auto section_model_hash { model_hash_.value(section) };

    auto model { section_model_hash.value(node_id, nullptr) };
    if (!model)
        return;

    connect(this, &SignalStation::SUpdateBalance, model, &TableModel::RUpdateBalance, Qt::SingleShotConnection);
    emit SUpdateBalance(node_id, trans_id);
}
