#ifndef TREEMODELPRODUCT_H
#define TREEMODELPRODUCT_H

#include "tree/model/abstracttreemodel.h"
#include "widget/tablewidget.h"

class TreeModelProduct final : public AbstractTreeModel {
    Q_OBJECT

public:
    TreeModelProduct(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelProduct() override = default;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void UpdateNode(const Node* tmp_node) override;

private:
    bool UpdateUnitPrice(Node* node, double value, CString& field = UNIT_PRICE);
    bool UpdateCommission(Node* node, double value, CString& field = COMMISSION);
};

#endif // TREEMODELPRODUCT_H
