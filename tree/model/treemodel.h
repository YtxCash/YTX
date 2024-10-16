#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QComboBox>
#include <QMimeData>

#include "component/constvalue.h"
#include "component/using.h"
#include "database/sqlite/sqlite.h"
#include "widget/tablewidget/tablewidget.h"

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~TreeModel();

protected:
    TreeModel(SPSqlite sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);

    TreeModel() = delete;
    TreeModel(const TreeModel&) = delete;
    TreeModel& operator=(const TreeModel&) = delete;
    TreeModel(TreeModel&&) = delete;
    TreeModel& operator=(TreeModel&&) = delete;

signals:
    // send to related table model
    void SRule(int node_id, bool rule);

    // send to its view
    void SResizeColumnToContents(int column);

    // send to search dialog
    void SSearch();

    // send to mainwindow
    void SUpdateName(const Node* node);
    void SUpdateDSpinBox();

    // send to InsertNodeOrder and TableWidgetOrder
    void SUpdateOrderPartyEmployee();

public slots:
    // receive from sqlite
    bool RUpdateMultiLeafTotal(const QList<int>& node_list);
    bool RRemoveNode(int node_id);

    // receive from related table model
    virtual void RUpdateLeafValueOne(int /*node_id*/, double /*diff*/, CString& /*node_field*/) { };
    virtual void RUpdateLeafValue(
        int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double settled_diff = 0.0);

    // receive from table model, member function
    void RSearch() { emit SSearch(); }

public:
    // virtual functions
    virtual bool RemoveNode(int row, const QModelIndex& parent = QModelIndex());
    virtual bool InsertNode(int row, const QModelIndex& parent, Node* node);
    virtual void UpdateNode(const Node* tmp_node);
    virtual void UpdateBaseUnit(int default_unit);

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
    void SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const;

    int Employee(int node_id) const { return GetValue(node_id, &Node::employee); }
    int Unit(int node_id) const { return GetValue(node_id, &Node::unit); }
    const QString& Name(int node_id) const { return GetValue(node_id, &Node::name); }
    const QString& Color(int node_id) const { return GetValue(node_id, &Node::date_time); }
    bool Branch(int node_id) const { return GetValue(node_id, &Node::branch); }
    bool Rule(int node_id) const { return GetValue(node_id, &Node::rule); }
    double InitialTotal(int node_id) const { return GetValue(node_id, &Node::initial_total); }
    double FinalTotal(int node_id) const { return GetValue(node_id, &Node::final_total); }
    double First(int node_id) const { return GetValue(node_id, &Node::first); }

    bool ChildrenEmpty(int node_id) const;
    bool Contains(int node_id) const { return node_hash_.contains(node_id); }

    QString GetPath(int node_id) const;
    QModelIndex GetIndex(int node_id) const;
    void SetNodeShadow(NodeShadow* node_shadow, int node_id) const;
    void SetNodeShadow(NodeShadow* node_shadow, Node* node) const;

    void LeafPathSpecificUnit(QComboBox* combo, int unit, UnitFilterMode unit_filter_mode) const;
    void LeafPathExcludeID(QComboBox* combo, int exclude_id) const;
    void LeafPathBranchPath(QComboBox* combo) const;

    QStringList ChildrenName(int node_id, int exclude_child) const;

    void SetParent(Node* node, int parent_id) const;
    void CopyNode(Node* tmp_node, int node_id) const;

    void UpdateSeparator(CString& old_separator, CString& new_separator);

protected:
    // virtual functions
    virtual void ConstructTree();
    virtual bool UpdateRule(Node* node, bool value);
    virtual bool UpdateUnit(Node* node, int value);
    virtual bool UpdateName(Node* node, CString& value);
    virtual bool IsReferenced(int node_id, CString& message) const;
    virtual void UpdateAncestorValue(
        Node* node, double initial_diff, double final_diff, double amount_diff = 0.0, double discount_diff = 0.0, double settled_diff = 0.0);

    // member functions

    bool IsDescendant(Node* lhs, Node* rhs) const;
    void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare) const;

    Node* GetNodeByID(int node_id) const;
    Node* GetNodeByIndex(const QModelIndex& index) const;
    QString ConstructPath(const Node* node) const;

    // jsus store leaf's total into sqlite3 table, ignore branch's total
    void UpdatePath(const Node* node);
    void UpdateBranchUnit(Node* node) const;
    bool UpdateBranch(Node* node, bool new_value);

    void InitializeRoot(int default_unit);
    void ShowTemporaryTooltip(CString& message, int duration = 3000) const;
    bool HasChildren(Node* node, CString& message) const;
    bool IsOpened(int node_id, CString& message) const;

protected:
    template <typename T> const T& GetValue(int node_id, T Node::* member) const
    {
        if (auto it = node_hash_.constFind(node_id); it != node_hash_.constEnd())
            return it.value()->*member;

        // If the node_id does not exist, return a static empty object to ensure a safe default value
        // Examples:
        // double InitialTotal(int node_id) const { return GetValue(node_id, &Node::initial_total); }
        // double FinalTotal(int node_id) const { return GetValue(node_id, &Node::final_total); }
        // Note: In the SetStatus() function of TreeWidget,
        // a node_id of 0 may be passed, so empty{} is needed to prevent illegal access
        static const T empty {};
        return empty;
    }

    template <typename T> bool UpdateField(Node* node, const T& value, CString& field, T Node::* member) const
    {
        if constexpr (std::is_floating_point_v<T>) {
            if (std::abs(node->*member - value) < TOLERANCE)
                return false;
        } else {
            if (node->*member == value)
                return false;
        }

        node->*member = value;
        sql_->UpdateField(info_.node, value, field, node->id);
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
