#ifndef TABLEMODELSTAKEHOLDER_H
#define TABLEMODELSTAKEHOLDER_H

#include "tablemodel.h"

class TableModelStakeholder final : public TableModel {
    Q_OBJECT

public:
    TableModelStakeholder(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);
    ~TableModelStakeholder() override = default;

public slots:
    // receive from sql
    void RRemoveMultiTrans(const QMultiHash<int, int>& /*node_trans*/) override { return; }

public:
    // implemented functions
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool RemoveTrans(int row, const QModelIndex& parent = QModelIndex()) override;

protected:
    bool RemoveMulti(const QList<int>& trans_id_list) override; // just remove trnas_shadow, keep related trans
    bool InsertMulti(int node_id, const QList<int>& trans_id_list) override;

private:
    bool UpdateRatio(TransShadow* trans_shadow, double value) override;
    bool UpdateCommission(TransShadow* trans_shadow, double value);
};

#endif // TABLEMODELSTAKEHOLDER_H
