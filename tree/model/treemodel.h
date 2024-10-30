#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>
#include <QStandardItemModel>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/using.h"
#include "tree/node.h"

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = nullptr);
    virtual ~TreeModel() = default;

protected:
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

    // send to specificunit delegate, table combo delegate, InsertNodeOrder and TableWidgetOrder
    void SUpdateComboModel();

public slots:
    // receive from sqlite
    bool RRemoveNode(int node_id);

    // default impl for order, stakeholder
    virtual bool RUpdateMultiLeafTotal(const QList<int>& /*node_list*/) { return {}; }

    // receive from related table model
    // default impl for finance, stakeholder, product
    virtual void RUpdateLeafValueOne(int /*node_id*/, double /*diff*/, CString& /*node_field*/) { }

    // default impl for stakeholder
    virtual void RUpdateLeafValue(int /*node_id*/, double /*initial_debit_diff*/, double /*initial_credit_diff*/, double /*final_debit_diff*/,
        double /*final_credit_diff*/, double /*settled_diff*/)
    {
    }

    // receive from table model, member function
    void RSearch() { emit SSearch(); }

public:
    // Qt's
    // Default implementations
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }
    QStringList mimeTypes() const override { return QStringList { NODE_ID }; }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& /*parent*/) const override
    {
        return data && data->hasFormat(NODE_ID) && action != Qt::IgnoreAction;
    }

    // Ytx's
    // Default implementations
    virtual double InitialTotal(int /*node_id*/) const { return {}; }
    virtual double FinalTotal(int /*node_id*/) const { return {}; }

    virtual QStringList ChildrenName(int /*node_id*/, int /*exclude_child*/) const { return {}; }
    virtual bool Branch(int /*node_id*/) const { return {}; }

    virtual void SetNodeShadow(NodeShadow* /*node_shadow*/, int /*node_id*/) const { }
    virtual void SetNodeShadow(NodeShadow* /*node_shadow*/, Node* /*node*/) const { }
    virtual void LeafPathSpecificUnit(QStandardItemModel* /*combo_model*/, int /*unit*/, UnitFilterMode /*unit_filter_mode*/) const { }
    virtual void LeafPathBranchPath(QStandardItemModel* /*combo_model*/) const { }
    virtual void CopyNode(Node* /*tmp_node*/, int /*node_id*/) const { }
    virtual void LeafPathExcludeID(QStandardItemModel* /*combo_model*/, int /*exclude_id*/) const { }
    virtual void UpdateNode(const Node* /*tmp_node*/) { }

    // Core pure virtual functions
    virtual void UpdateDefaultUnit(int default_unit) = 0;
    virtual void UpdateSeparator(CString& old_separator, CString& new_separator) = 0;
    virtual void SetParent(Node* node, int parent_id) const = 0;
    virtual void SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const = 0;

    virtual bool Contains(int node_id) const = 0;
    virtual bool ChildrenEmpty(int node_id) const = 0;
    virtual bool Rule(int node_id) const = 0;
    virtual bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) = 0;
    virtual bool InsertNode(int row, const QModelIndex& parent, Node* node) = 0;

    virtual QModelIndex GetIndex(int node_id) const = 0;
    virtual int Unit(int node_id) const = 0;
    virtual QString GetPath(int node_id) const = 0;
    virtual const QString& Name(int node_id) const = 0;

protected:
    // Core pure virtual functions
    virtual bool UpdateUnit(Node* node, int value) = 0;
    virtual bool UpdateName(Node* node, CString& value) = 0;
    virtual Node* GetNodeByIndex(const QModelIndex& index) const = 0;

    // Default implementations
    virtual bool IsReferenced(int /*node_id*/, CString& /*message*/) const { return {}; }
    virtual bool UpdateBranch(Node* /*node*/, bool /*value*/) { return {}; }
    virtual bool UpdateRule(Node* /*node*/, bool /*value*/) { return {}; }

    virtual void ConstructTree() { };
    virtual void UpdateAncestorValue(
        Node* /*node*/, double /*initial_diff*/, double /*final_diff*/, double /*amount_diff*/, double /*discount_diff*/, double /*settled_diff*/) { };
};

using PTreeModel = QPointer<TreeModel>;

#endif // TREEMODEL_H
