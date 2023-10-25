#ifndef SIGNALMANAGER_H
#define SIGNALMANAGER_H

#include "../table/tablemodel.h"

class SignalManager : public QObject {
    Q_OBJECT

public:
    static SignalManager& Instance();
    void RegisterTableModel(int node_id, const TableModel* model);
    void DeregisterTableModel(int node_id);

signals:
    void SendUpdate(const Transaction& transaction);
    void SendCopy(int node_id, const Transaction& transaction);
    void SendRemove(int node_id, int id);

    void SendReplace(int old_node_id, int new_node_id);
    void SendReload(int node_id);

public slots:
    void ReceiveUpdate(const Transaction& transaction);
    void ReceiveCopy(int node_id, const Transaction& transaction);
    void ReceiveRemove(int node_id, int id);

    void ReceiveReloadAll(int node_id);

private:
    SignalManager() = default;

private:
    QHash<int, const TableModel*> opened_model_ {};
};

#endif // SIGNALMANAGER_H
