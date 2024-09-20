#ifndef SQLITE_H
#define SQLITE_H

// All virtual functions' default implementations are for finance/task section.

#include <QObject>
#include <QSqlDatabase>

#include "component/enumclass.h"
#include "component/info.h"
#include "component/using.h"
#include "table/trans.h"
#include "tree/node.h"

class Sqlite : public QObject {
    Q_OBJECT

public:
    Sqlite(CInfo& info, QObject* parent = nullptr);
    virtual ~Sqlite();

signals:
    // send to all table model
    void SRemoveMulti(const QMultiHash<int, int>& node_trans);
    // send to signal station
    void SMoveMulti(Section section, int old_node_id, int new_node_id, const QList<int>& trans_id_list);
    void SRemoveMultiReferences(Section target, int node_id, const QList<int>& trans_id_list);
    // send to tree model
    void SUpdateMultiTotal(const QList<int>& node_id_list);
    void SRemoveNode(int node_id);
    // send to mainwindow
    void SFreeView(int node_id);
    // send to sql itsself
    void SReplaceReferences(Section origin, int old_node_id, int new_node_id);

public slots:
    // receive from remove node dialog
    virtual bool RRemoveMulti(int node_id);
    virtual bool RReplaceMulti(int old_node_id, int new_node_id);
    // receive from sql
    virtual bool RReplaceReferences(Section origin, int old_node_id, int new_node_id)
    {
        Q_UNUSED(old_node_id)
        Q_UNUSED(new_node_id)
        Q_UNUSED(origin)
        return false;
    }

public:
    // tree
    virtual bool BuildTree(NodeHash& node_hash);
    virtual bool InsertNode(int parent_id, Node* node);
    virtual void NodeLeafTotal(Node* node);
    virtual bool UpdateNodeSimple(const Node* node);
    virtual bool RemoveNode(int node_id, bool branch = false);
    virtual bool NodeInternalReferences(int node_id) const;
    virtual bool NodeExternalReferences(int node_id) const
    {
        Q_UNUSED(node_id)
        return false;
    };

    bool DragNode(int destination_node_id, int node_id);

    // table
    virtual void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id);
    virtual void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list);
    virtual bool InsertTransShadow(TransShadow* trans_shadow);

    bool RemoveTrans(int trans_id);
    bool UpdateTrans(int trans_id);
    bool UpdateCheckState(CString& column, CVariant& value, Check state);

    TransShadow* AllocateTransShadow();
    QHash<int, Trans*>* TransHash() { return &trans_hash_; } // 需要改变设计

    // common
    bool UpdateField(CString& table, CVariant& new_value, CString& field, int id);

protected:
    // tree
    virtual void BuildNodeHash(QSqlQuery& query, NodeHash& node_hash);

    bool DBTransaction(std::function<bool()> function);
    void ReadRelationship(QSqlQuery& query, const NodeHash& node_hash);
    void WriteRelationship(QSqlQuery& query, int node_id, int parent_id);

    // table
    virtual void QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query);
    virtual QMultiHash<int, int> RelatedNodeTrans(int node_id) const;

    void Convert(Trans* trans, TransShadow* trans_shadow, bool left);

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

using SPSqlite = QSharedPointer<Sqlite>;

#endif // SQLITE_H
