#ifndef TREEMODELPRODUCT_H
#define TREEMODELPRODUCT_H

#include "tree/model/treemodel.h"

class TreeModelProduct final : public TreeModel {
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

protected:
    bool IsReferenced(int node_id, CString& message) override;
    bool UpdateUnit(Node* node, int value) override;
};

#endif // TREEMODELPRODUCT_H
