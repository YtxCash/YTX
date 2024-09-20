#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include "tree/model/abstracttreemodel.h"
#include "widget/tablewidget.h"

class TreeModelOrder final : public AbstractTreeModel {
    Q_OBJECT

public:
    TreeModelOrder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelOrder() override = default;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    void UpdateNode(const Node* tmp_node) override;

protected:
    void ConstructTree() override;
    bool UpdateNodeRule(Node* node, bool value) override; // bill = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateName(Node* node, CString& value) override;

private:
    bool UpdateParty(Node* node, int value);
    bool UpdateEmployee(Node* node, int value);
    bool UpdateDateTime(Node* node, CString& value);
    bool UpdateDiscount(Node* node, double value);
    bool UpdateLocked(Node* node, bool value); // unlocked = 0, locked = 1

    void UpdateBranchTotalUp(const Node* node, int first, double second, double discount, double initial_total, double final_total);
    void UpdateBranchTotalDown(Node* node);
    void UpdateNodeValues(Node* node, int first, double second, double discount, double initial_total, double final_total, int coefficient);

    void RefreshAncestorData(const QModelIndex& index, int column_start, int column_end);
};

#endif // TREEMODELORDER_H
