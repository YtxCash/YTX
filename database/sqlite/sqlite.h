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
    virtual ~Sqlite();

protected:
    Sqlite(CInfo& info, QObject* parent = nullptr);

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
    bool BuildTree(NodeHash& node_hash);
    virtual QString BuildTreeQueryString() const;

    virtual bool InsertNode(int parent_id, Node* node);
    virtual void NodeLeafTotal(Node* node);

    bool RemoveNode(int node_id, bool branch = false);
    virtual QString RemoveNodeQueryStringSecond() const;

    bool InternalReference(int node_id) const;
    virtual QString InternalReferenceQueryString() const;

    virtual bool ExternalReference(int /*node_id*/) const { return false; }

    bool DragNode(int destination_node_id, int node_id);

    // table
    void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id);
    virtual QString BuildTransShadowListQueryString() const;

    void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list);
    virtual QString BuildTransShadowListRangQueryString(QStringList& list) const;

    virtual bool InsertTransShadow(TransShadow* trans_shadow);

    bool RemoveTrans(int trans_id);
    bool UpdateTrans(int trans_id);
    bool UpdateCheckState(CString& column, CVariant& value, Check state);

    TransShadow* AllocateTransShadow();
    QHash<int, Trans*>* TransHash() { return &trans_hash_; } // 需要改变设计

    // common
    bool UpdateField(CString& table, CVariant& value, CString& field, int id);

protected:
    // tree
    virtual void ReadNode(Node* node, const QSqlQuery& query);

    void BuildNodeHash(NodeHash& node_hash, QSqlQuery& query);
    bool DBTransaction(std::function<bool()> function);
    void ReadRelationship(const NodeHash& node_hash, QSqlQuery& query);
    void WriteRelationship(int node_id, int parent_id, QSqlQuery& query);

    // table
    virtual void ReadTrans(Trans* trans, const QSqlQuery& query);

    void QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query);
    void ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left);
    QMultiHash<int, int> RelatedNodeTrans(int node_id) const;

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

using SPSqlite = QSharedPointer<Sqlite>;

#endif // SQLITE_H
