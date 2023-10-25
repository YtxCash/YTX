#ifndef TABLEMODELORDER_H
#define TABLEMODELORDER_H

#include "tablemodel.h"

class TableModelOrder final : public TableModel {
    Q_OBJECT

public:
    TableModelOrder(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);
    ~TableModelOrder() override = default;

public:
    // implemented functions
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
};

#endif // TABLEMODELORDER_H
