#ifndef TABLEMODELORDER_H
#define TABLEMODELORDER_H

#include "tablemodel.h"
#include "tree/model/treemodel.h"

class TableModelOrder final : public TableModel {
    Q_OBJECT

public:
    TableModelOrder(SPSqlite sql, bool rule, int node_id, CInfo& info, TreeModel* product, QObject* parent = nullptr);
    ~TableModelOrder() override = default;

public:
    // implemented functions
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

private:
    bool UpdateInsideProduct(TransShadow* trans_shadow, int value);

    bool UpdateUnitPrice(TransShadow* trans_shadow, double value);
    bool UpdateDiscountPrice(TransShadow* trans_shadow, double value);
    bool UpdateSecond(TransShadow* trans_shadow, double value);
    bool UpdateFirst(TransShadow* trans_shadow, double value);
    bool UpdateCode(TransShadow* trans_shadow, CString& value);

private:
    TreeModel* product_ {};
};

#endif // TABLEMODELORDER_H
