#ifndef TREEMODELTASK_H
#define TREEMODELTASK_H

#include "treemodel.h"
#include "treemodelhelper.h"

class TreeModelTask final : public TreeModel {
    Q_OBJECT

public:
    TreeModelTask(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelTask() override;

public slots:
    void RUpdateLeafValueTO(int node_id, double diff, CString& node_field) override; // unit_cost
    void RUpdateLeafValueFPTO(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff,
        double settled_diff = 0.0) override;
    void RUpdateMultiLeafTotalFPT(const QList<int>& node_list) override;

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

    void UpdateNodeFPTS(const Node* tmp_node) override;
    bool UpdateBranchFPTS(Node* node, bool value) override;
    void UpdateSeparatorFPTS(CString& old_separator, CString& new_separator) override;
    void CopyNodeFPTS(Node* tmp_node, int node_id) const override;
    void SetParent(Node* node, int parent_id) const override;
    QStringList ChildrenNameFPTS(int node_id, int exclude_child) const override;
    QString GetPath(int node_id) const override;
    void LeafPathBranchPathFPT(QStandardItemModel* combo_model) const override;
    void LeafPathExcludeIDFPTS(QStandardItemModel* combo_model, int exclude_id) const override;
    QModelIndex GetIndex(int node_id) const override;
    bool Contains(int node_id) const override { return node_hash_.contains(node_id); }
    bool ChildrenEmpty(int node_id) const override;
    double InitialTotalFPT(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::initial_total); }
    double FinalTotalFPT(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::final_total); }
    int Unit(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::unit); }
    QString Name(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::name); }
    bool BranchFPTS(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::branch); }
    bool Rule(int node_id) const override { return TreeModelHelper::GetValue(node_hash_, node_id, &Node::rule); }
    void SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    void UpdateDefaultUnit(int default_unit) override { root_->unit = default_unit; }

protected:
    bool IsReferencedFPTS(int node_id, CString& message) const override;
    bool UpdateName(Node* node, CString& value) override;
    bool UpdateUnit(Node* node, int value) override;
    bool UpdateRuleFPTO(Node* node, bool value) override;
    void ConstructTreeFPTS() override;
    Node* GetNodeByIndex(const QModelIndex& index) const override;

private:
    Sqlite* sql_ {};
    Node* root_ {};

    NodeHash node_hash_ {};
    StringHash leaf_path_ {};
    StringHash branch_path_ {};

    CInfo& info_;
    CTableHash& table_hash_;
    CString& separator_;
};

#endif // TREEMODELTASK_H
