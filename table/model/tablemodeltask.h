#ifndef TABLEMODELTASK_H
#define TABLEMODELTASK_H

#include "tablemodel.h"

class TableModelTask final : public TableModel {
    Q_OBJECT

public:
    TableModelTask(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelTask() override = default;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    bool UpdateDebit(TransShadow* trans_shadow, double value) override;
    bool UpdateCredit(TransShadow* trans_shadow, double value) override;
    bool UpdateRatio(TransShadow* trans_shadow, double value) override;
};

#endif // TABLEMODELTASK_H
