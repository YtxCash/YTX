#ifndef TREEMODEL_H
#define TREEMODEL_H

// All virtual functions' default implementations are for finance/task section.

#include <QAbstractItemModel>
#include <QComboBox>
#include <QMimeData>

#include "component/constvalue.h"
#include "component/using.h"
#include "database/sqlite/sqlite.h"
#include "widget/tablewidget.h"

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    TreeModel(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    virtual ~TreeModel();

signals:
    // send to related table model
    void SNodeRule(int node_id, bool node_rule);

    // send to its view
    void SResizeColumnToContents(int column);

    // send to search dialog
    void SSearch();

    // send to mainwindow
    void SUpdateName(const Node* node);
    void SUpdateDSpinBox();

public slots:
    // receive from table sql
    virtual bool RUpdateMultiTotal(const QList<int>& node_list);
    virtual bool RRemoveNode(int node_id);

    // receive from related table model
    virtual void RUpdateOneTotal(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff);

    // receive from table model, member function
    void RSearch() { emit SSearch(); }

public:
    // virtual functions
    virtual bool RemoveNode(int row, const QModelIndex& parent = QModelIndex());
    virtual bool InsertNode(int row, const QModelIndex& parent, Node* node);
    virtual void UpdateNode(const Node* tmp_node);
    virtual void UpdateBaseUnit(int base_unit);

    // implemented functions
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    // inline functions
    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }
    QStringList mimeTypes() const override { return QStringList { "node/id" }; }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override
    {
        Q_UNUSED(row)
        Q_UNUSED(column)
        Q_UNUSED(parent)

        return data && data->hasFormat(NODE_ID) && action != Qt::IgnoreAction;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return info_.tree_header.at(section);

        return QVariant();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return info_.tree_header.size();
    }

    // member functions
    void NodeList(QList<const Node*>& node_list, const QList<int>& id_list) const;

    int Employee(int node_id) const { return GetValueOrDefault(node_id, &Node::employee, 0); }
    int Unit(int node_id) const { return GetValueOrDefault(node_id, &Node::unit, 0); }
    QString Name(int node_id) const { return GetValueOrDefault(node_id, &Node::name, QString()); }
    bool Branch(int node_id) const { return GetValueOrDefault(node_id, &Node::branch, false); }
    bool NodeRule(int node_id) const { return GetValueOrDefault(node_id, &Node::node_rule, false); }
    double InitialTotal(int node_id) const { return GetValueOrDefault(node_id, &Node::initial_total, 0.0); }
    double FinalTotal(int node_id) const { return GetValueOrDefault(node_id, &Node::final_total, 0.0); }

    bool ChildrenEmpty(int node_id) const;
    bool Contains(int node_id) const { return node_hash_.contains(node_id); }

    QString GetPath(int node_id) const { return leaf_path_.value(node_id, branch_path_.value(node_id, QString())); }
    QModelIndex GetIndex(int node_id) const;

    void ComboPathUnit(QComboBox* combo, int unit) const;
    void ComboPathLeaf(QComboBox* combo, int exclude) const;
    void ComboPathLeafBranch(QComboBox* combo) const;

    void ChildrenName(QStringList& list, int node_id, int exclude_child) const;

    void SetParent(Node* node, int parent_id) const;
    void CopyNode(Node* tmp_node, int node_id) const;

    void UpdateSeparator(CString& old_separator, CString& new_separator);

protected:
    // virtual functions
    virtual bool UpdateNodeRule(Node* node, bool value);
    virtual bool UpdateUnit(Node* node, int value);

    // member functions
    void ConstructTree(Node* root);
    QString ConstructPath(const Node* node, const Node* root) const;

    bool IsDescendant(Node* lhs, Node* rhs) const;
    void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare);

    Node* GetNodeByID(int node_id) const { return node_hash_.value(node_id, nullptr); }
    Node* GetNodeByIndex(const QModelIndex& index) const;

    // jsus store leaf's total into sqlite3 table, ignore branch's total
    bool UpdateLeafTotal(const Node* node, CString& initial, CString& final = QString());
    void UpdateBranchTotal(const Node* node, const Node* root, double initial_diff, double final_diff);
    void UpdatePath(const Node* node, const Node* root);

    bool UpdateCode(Node* node, CString& new_value, CString& field = CODE);
    bool UpdateDescription(Node* node, CString& new_value, CString& field = DESCRIPTION);
    bool UpdateNote(Node* node, CString& new_value, CString& field = NOTE);
    bool UpdateBranch(Node* node, bool new_value);
    void UpdateBranchUnit(Node* node) const;
    bool UpdateName(Node* node, Node* root, CString& new_value);

    void InitializeRoot(Node*& root, int base_unit);

protected:
    template <typename T> std::optional<T> GetValue(int node_id, T Node::*member) const
    {
        if (auto it = node_hash_.find(node_id); it != node_hash_.end())
            return (*it)->*member;

        return std::nullopt;
    }

    template <typename T> T GetValueOrDefault(int node_id, T Node::*member, const T& default_value = T {}) const
    {
        return GetValue(node_id, member).value_or(default_value);
    }

    template <typename T> bool UpdateField(Node* node, const T& new_value, CString& field, T Node::*member)
    {
        if constexpr (std::is_floating_point_v<T>) {
            static const T tolerance = static_cast<T>(std::pow(10, -9));
            if (std::abs(node->*member - new_value) < tolerance)
                return false;
        } else {
            if (node->*member == new_value)
                return false;
        }

        node->*member = new_value;
        sql_->UpdateField(info_.node, field, new_value, node->id);
        return true;
    }

protected:
    SPSqlite sql_ {};
    Node* root_ {};

    NodeHash node_hash_ {};
    StringHash leaf_path_ {};
    StringHash branch_path_ {};

    CInfo& info_;
    CTableHash& table_hash_;
    CString& separator_;
};

#endif // TREEMODEL_H
