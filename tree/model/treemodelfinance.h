#ifndef TREEMODELFINANCE_H
#define TREEMODELFINANCE_H

#include "tree/model/treemodel.h"
#include "treemodelhelper.h"

class TreeModelFinance final : public TreeModel {
    Q_OBJECT

public:
    TreeModelFinance(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelFinance() override;

public slots:
    void RUpdateLeafValueFPTO(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff,
        double settled_diff = 0.0) override;
    void RUpdateMultiLeafTotalFPT(const QList<int>& node_list) override;

public:
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    void UpdateNodeFPTS(const Node* tmp_node) override;
    void UpdateDefaultUnit(int default_unit) override;

    QModelIndex parent(const QModelIndex& index) const override;
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

    void UpdateSeparatorFPTS(CString& old_separator, CString& new_separator) override;
    void CopyNodeFPTS(Node* tmp_node, int node_id) const override;
    void SetParent(Node* node, int parent_id) const override;
    QStringList ChildrenNameFPTS(int node_id, int exclude_child) const override;
    QString GetPath(int node_id) const override;
    void LeafPathBranchPathFPT(QStandardItemModel* combo_model) const override;
    void LeafPathExcludeIDFPT(QStandardItemModel* combo_model, int exclude_id) const override;
    void LeafPathSpecificUnitExcludeIDFPTS(QStandardItemModel* combo_model, int unit, int exclude_id) const override;
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

protected:
    bool UpdateRuleFPTO(Node* node, bool value) override;
    bool UpdateUnit(Node* node, int value) override;
    bool UpdateName(Node* node, CString& value) override;
    bool IsReferencedFPTS(int node_id, CString& message) const override;

    Node* GetNodeByIndex(const QModelIndex& index) const override;
    bool UpdateBranchFPTS(Node* node, bool value) override;
    void ConstructTree() override;

private:
    Sqlite* sql_ {};
    Node* root_ {};
    QMutex mutex_ {};

    NodeHash node_hash_ {};
    StringHash leaf_path_ {};
    StringHash branch_path_ {};

    CInfo& info_;
    CTableHash& table_hash_;
    CString& separator_;
};

#endif // TREEMODELFINANCE_H
