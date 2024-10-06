#ifndef TABLEMODELPRODUCT_H
#define TABLEMODELPRODUCT_H

#include "tablemodel.h"

class TableModelProduct final : public TableModel {
    Q_OBJECT

public:
    TableModelProduct(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelProduct() override = default;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;

protected:
    bool UpdateDebit(TransShadow* trans_shadow, double value) override;
    bool UpdateCredit(TransShadow* trans_shadow, double value) override;
    bool UpdateRatio(TransShadow* trans_shadow, double value) override;
};

#endif // TABLEMODELPRODUCT_H
