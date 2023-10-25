#include "../globalmanager/signalmanager.h"
#include <QDebug>

SignalManager& SignalManager::Instance()
{
    static SignalManager instance;
    return instance;
}

void SignalManager::RegisterTableModel(int node_id, const TableModel* model)
{
    opened_model_.insert(node_id, model);
}

void SignalManager::DeregisterTableModel(int node_id)
{
    opened_model_.remove(node_id);
}

void SignalManager::ReceiveUpdate(const Transaction& transaction)
{
    int transfer_id = transaction.transfer;

    if (opened_model_.contains(transfer_id)) {
        auto model = opened_model_.value(transfer_id);

        connect(this, &SignalManager::SendUpdate, model, &TableModel::ReceiveUpdate, Qt::UniqueConnection);
        emit SendUpdate(transaction);
        disconnect(this, &SignalManager::SendUpdate, model, &TableModel::ReceiveUpdate);
    }
}

void SignalManager::ReceiveCopy(int node_id, const Transaction& transaction)
{
    int transfer_id = transaction.transfer;

    if (opened_model_.contains(transfer_id)) {
        auto model = opened_model_.value(transfer_id);

        connect(this, &SignalManager::SendCopy, model, &TableModel::ReceiveCopy, Qt::UniqueConnection);
        emit SendCopy(node_id, transaction);
        disconnect(this, &SignalManager::SendCopy, model, &TableModel::ReceiveCopy);
    }
}

void SignalManager::ReceiveRemove(int node_id, int id)
{
    if (opened_model_.contains(node_id)) {
        auto model = opened_model_.value(node_id);

        connect(this, &SignalManager::SendRemove, model, &TableModel::ReceiveRemove, Qt::UniqueConnection);
        emit SendRemove(node_id, id);
        disconnect(this, &SignalManager::SendRemove, model, &TableModel::ReceiveRemove);
    }
}

void SignalManager::ReceiveReloadAll(int node_id)
{
    for (auto it = opened_model_.cbegin(); it != opened_model_.cend(); ++it) {
        int id = it.key();
        if (node_id != id) {
            auto model = it.value();
            connect(this, &SignalManager::SendReload, model, &TableModel::ReceiveReload, Qt::UniqueConnection);
            emit SendReload(id);
            disconnect(this, &SignalManager::SendReload, model, &TableModel::ReceiveReload);
        }
    }
}
