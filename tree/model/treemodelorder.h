#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include "tree/model/treemodel.h"

class TreeModelOrder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelOrder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelOrder() override = default;

signals:
    void SUpdateLocked(int node_id, bool checked);

public slots:
    bool RUpdateStakeholderReference(int old_node_id, int new_node_id);
    void RUpdateLocked(int node_id, bool checked);
    void RUpdateLeafValueOrder(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;

protected:
    void ConstructTree() override;
    bool UpdateRule(Node* node, bool value) override; // charge = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateName(Node* node, CString& value) override;

private:
    void UpdateAncestorValue(Node* node, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff);
    bool UpdateLocked(Node* node, bool value);
};

#endif // TREEMODELORDER_H
