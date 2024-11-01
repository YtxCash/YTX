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

public slots:
    // receive from TableModel
    void RAppendOneTrans(Section section, const TransShadow* trans_shadow);
    void RRemoveOneTrans(Section section, int node_id, int trans_id);
    void RUpdateBalance(Section section, int node_id, int trans_id);

    // receive from SqliteStakeholder
    void RAppendPrice(Section section, TransShadow* trans_shadow);

    // receive from TreeModel
    void RRule(Section section, int node_id, bool rule);

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
