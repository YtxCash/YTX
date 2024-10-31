#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include <QDate>

#include "database/sqlite/sqliteorder.h"
#include "tree/model/treemodel.h"
#include "treemodelhelper.h"

class TreeModelOrder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelOrder() override;

signals:
    void SUpdateData(int node_id, TreeEnumOrder column, const QVariant& value);

public slots:
    void RUpdateLeafValueOne(int node_id, double diff, CString& node_field) override; // first
    void RUpdateLeafValue(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff) override;

    bool RUpdateStakeholderReference(int old_node_id, int new_node_id);
    void RUpdateLocked(int node_id, bool checked);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    int columnCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return info_.tree_header.size(); }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        return TreeModelHelper::headerData(info_, section, orientation, role);
    }

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;

    void ConstructTreeOrder(const QDate& start_date, const QDate& end_date);
    void UpdateSeparator(CString& old_separator, CString& new_separator) override;
    void SetParent(Node* node, int parent_id) const override;
    QString GetPath(int node_id) const override;
    void SetNodeShadow(NodeShadow* node_shadow, int node_id) const override;
    void SetNodeShadow(NodeShadow* node_shadow, Node* node) const override;
    QModelIndex GetIndex(int node_id) const override;
    bool Contains(int node_id) const override { return node_hash_.contains(node_id); }
    bool ChildrenEmpty(int node_id) const override;
    int Unit(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::unit); }
    QString Name(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::name); }
    bool Rule(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::rule); }
    void SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const override;
    void UpdateDefaultUnit(int default_unit) override { root_->unit = default_unit; }

protected:
    bool UpdateRule(Node* node, bool value) override; // charge = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateName(Node* node, CString& value) override;
    void UpdateAncestorValue(
        Node* node, double first_diff, double second_diff = 0.0, double amount_diff = 0.0, double discount_diff = 0.0, double settled_diff = 0.0) override;

    Node* GetNodeByIndex(const QModelIndex& index) const override;

private:
    bool UpdateLocked(Node* node, bool value);

private:
    SqliteOrder* sql_ {};
    Node* root_ {};

    NodeHash node_hash_ {};
    StringHash leaf_path_ {};
    StringHash branch_path_ {};

    CInfo& info_;
    CTableHash& table_hash_;
    CString& separator_;
};

#endif // TREEMODELORDER_H
