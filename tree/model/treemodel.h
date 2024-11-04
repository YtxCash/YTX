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
    // send to SignalStation
    void SRule(Section seciton, int node_id, bool rule);

    // send to its view
    void SResizeColumnToContents(int column);

    // send to Search dialog
    void SSearch();

    // send to Mainwindow
    void SUpdateName(const Node* node);
    void SUpdateDSpinBox();

    // send to SpecificUnit delegate, TableCombo delegate, EditNodeOrder and TableWidgetOrder
    void SUpdateComboModel();

public slots:
    // receive from Sqlite
    bool RRemoveNode(int node_id);
    virtual void RUpdateMultiLeafTotalFPT(const QList<int>& /*node_list*/) { }

    // receive from  TableModel
    void RSearch() { emit SSearch(); }
    virtual void RUpdateLeafValueTO(int /*node_id*/, double /*diff*/, CString& /*node_field*/) { }
    virtual void RUpdateLeafValueFPTO(int /*node_id*/, double /*initial_debit_diff*/, double /*initial_credit_diff*/, double /*final_debit_diff*/,
        double /*final_credit_diff*/, double /*settled_diff*/)
    {
    }

    virtual void RUpdateStakeholderSO(int /*old_node_id*/, int /*new_node_id*/) { };

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
    virtual double InitialTotalFPT(int /*node_id*/) const { return {}; }
    virtual double FinalTotalFPT(int /*node_id*/) const { return {}; }

    virtual QStringList ChildrenNameFPTS(int /*node_id*/, int /*exclude_child*/) const { return {}; }
    virtual bool BranchFPTS(int /*node_id*/) const { return {}; }

    virtual void CopyNodeFPTS(Node* /*tmp_node*/, int /*node_id*/) const { }
    virtual void LeafPathBranchPathFPT(QStandardItemModel* /*combo_model*/) const { }
    virtual void LeafPathExcludeIDFPT(QStandardItemModel* /*combo_model*/, int /*exclude_id*/) const { }
    virtual void LeafPathSpecificUnitExcludeIDFPTS(QStandardItemModel* /*combo_model*/, int /*unit*/, int /*exclude_id*/) const { }
    virtual void LeafPathSpecificUnitPS(QStandardItemModel* /*combo_model*/, int /*unit*/, UnitFilterMode /*unit_filter_mode*/) const { }
    virtual void SetNodeShadowOrder(NodeShadow* /*node_shadow*/, int /*node_id*/) const { }
    virtual void SetNodeShadowOrder(NodeShadow* /*node_shadow*/, Node* /*node*/) const { }
    virtual void UpdateNodeFPTS(const Node* /*tmp_node*/) { }
    virtual void UpdateSeparatorFPTS(CString& /*old_separator*/, CString& /*new_separator*/) { };

    // Core pure virtual functions
    virtual void SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const = 0;
    virtual void SetParent(Node* node, int parent_id) const = 0;
    virtual void UpdateDefaultUnit(int default_unit) = 0;

    virtual bool ChildrenEmpty(int node_id) const = 0;
    virtual bool Contains(int node_id) const = 0;
    virtual bool InsertNode(int row, const QModelIndex& parent, Node* node) = 0;
    virtual bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) = 0;
    virtual bool Rule(int node_id) const = 0;

    virtual QModelIndex GetIndex(int node_id) const = 0;
    virtual QString Name(int node_id) const = 0;
    virtual QString GetPath(int node_id) const = 0;
    virtual int Unit(int node_id) const = 0;

protected:
    // Core pure virtual functions
    virtual Node* GetNodeByIndex(const QModelIndex& index) const = 0;
    virtual bool UpdateName(Node* node, CString& value) = 0;
    virtual bool UpdateUnit(Node* node, int value) = 0;
    virtual void ConstructTree() = 0;

    // Default implementations
    virtual bool IsReferencedFPTS(int /*node_id*/, CString& /*message*/) const { return {}; }
    virtual bool UpdateBranchFPTS(Node* /*node*/, bool /*value*/) { return {}; }
    virtual bool UpdateRuleFPTO(Node* /*node*/, bool /*value*/) { return {}; }
};

using PTreeModel = QPointer<TreeModel>;
using CTreeModel = const TreeModel;

#endif // TREEMODEL_H
