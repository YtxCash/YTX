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
    void SAppendOneTrans(const TransShadow* trans_shadow);
    void SRemoveOneTrans(int node_id, int trans_id);
    void SUpdateBalance(int node_id, int trans_id);

public slots:
    void RAppendOneTrans(Section section, const TransShadow* trans_shadow);
    void RRemoveOneTrans(Section section, int node_id, int trans_id);
    void RUpdateBalance(Section section, int node_id, int trans_id);

private:
    SignalStation() = default;
    ~SignalStation() { };

    SignalStation(const SignalStation&) = delete;
    SignalStation& operator=(const SignalStation&) = delete;
    SignalStation(SignalStation&&) = delete;
    SignalStation& operator=(SignalStation&&) = delete;

private:
    QHash<Section, QHash<int, const TableModel*>> model_hash_ {};
};

#endif // SIGNALSTATION_H
