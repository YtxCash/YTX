#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include "tree/model/treemodel.h"
#include "widget/tablewidget.h"

class TreeModelOrder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelOrder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelOrder() override = default;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    void UpdateNode(const Node* tmp_node) override;

protected:
    bool UpdateNodeRule(Node* node, bool value) override;

    void IniTree(NodeHash& node_hash);

    void UpdateBranchTotal(const Node* node, double primary_diff, double secondary_diff, double initial_diff, double final_diff);
    bool UpdateLeafTotal(const Node* node); // jsus store leaf's total into sqlite3 table

    bool UpdateFirst(Node* node, int value);
    bool UpdateEmployee(Node* node, int value);
    bool UpdateSecond(Node* node, double value);
    bool UpdateDiscount(Node* node, double value);
    bool UpdateRefund(Node* node, bool value);
    bool UpdateDateTime(Node* node, CString& value);
    bool UpdateParty(Node* node, int value);
};

#endif // TREEMODELORDER_H
