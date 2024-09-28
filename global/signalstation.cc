#include "global/signalstation.h"

SignalStation& SignalStation::Instance()
{
    static SignalStation instance;
    return instance;
}

void SignalStation::RegisterModel(Section section, int node_id, const TableModel* model)
{
    if (!model_hash_.contains(section))
        model_hash_[section] = QHash<int, const TableModel*>();

    model_hash_[section].insert(node_id, model);
}

void SignalStation::DeregisterModel(Section section, int node_id) { model_hash_[section].remove(node_id); }

void SignalStation::RAppendOne(Section section, const TransShadow* trans_shadow)
{
    auto section_model_hash { model_hash_.value(section) };
    int related_node_id { *trans_shadow->related_node };

    auto model { section_model_hash.value(related_node_id, nullptr) };
    if (!model)
        return;

    connect(this, &SignalStation::SAppendOne, model, &TableModel::RAppendOne, Qt::SingleShotConnection);
    emit SAppendOne(trans_shadow);
}

void SignalStation::RRetrieveOne(Section section, TransShadow* trans_shadow)
{
    auto section_model_hash { model_hash_.value(section) };

    int related_node_id { *trans_shadow->related_node };
    auto related_model { section_model_hash.value(related_node_id, nullptr) };
    if (!related_model)
        return;

    connect(this, &SignalStation::SAppendOne, related_model, &TableModel::RAppendOne, Qt::SingleShotConnection);
    emit SAppendOne(trans_shadow);

    int node { *trans_shadow->node };
    auto model { section_model_hash.value(node, nullptr) };
    if (!model)
        return;

    connect(this, &SignalStation::SRetrieveOne, model, &TableModel::RRetrieveOne, Qt::SingleShotConnection);
    emit SRetrieveOne(trans_shadow);
}

void SignalStation::RRemoveOne(Section section, int node_id, int trans_id)
{
    auto section_model_hash { model_hash_.value(section) };

    auto model { section_model_hash.value(node_id, nullptr) };
    if (!model)
        return;

    connect(this, &SignalStation::SRemoveOne, model, &TableModel::RRemoveOne, Qt::SingleShotConnection);
    emit SRemoveOne(node_id, trans_id);
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

void SignalStation::RMoveMultiTrans(Section section, int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    auto section_model_hash { model_hash_.value(section) };

    auto old_model { section_model_hash.value(old_node_id, nullptr) };
    if (old_model) {
        connect(this, &SignalStation::SMoveMulti, old_model, &TableModel::RMoveMulti, Qt::SingleShotConnection);
        emit SMoveMulti(old_node_id, new_node_id, trans_id_list);
    }

    auto new_model { section_model_hash.value(new_node_id, nullptr) };
    if (new_model) {
        connect(this, &SignalStation::SMoveMulti, new_model, &TableModel::RMoveMulti, Qt::SingleShotConnection);
        emit SMoveMulti(old_node_id, new_node_id, trans_id_list);
    }
}
