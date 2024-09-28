#ifndef SQLITE_H
#define SQLITE_H

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
    void SRemoveMultiTrans(const QMultiHash<int, int>& node_trans);
    // send to signal station
    void SMoveMultiTrans(Section section, int old_node_id, int new_node_id, const QList<int>& trans_id_list);
    void SRemoveMultiReferences(Section target, int node_id, const QList<int>& trans_id_list);
    // send to tree model
    void SUpdateMultiNodeTotal(const QList<int>& node_id_list);
    void SRemoveNode(int node_id);
    // send to mainwindow
    void SFreeView(int node_id);
    // send to sql itsself
    void SUpdateProductReference(int old_node_id, int new_node_id);

public slots:
    // receive from remove node dialog
    bool RRemoveNode(int node_id);
    bool RReplaceNode(int old_node_id, int new_node_id);
    // receive from sql
    bool RUpdateProductReference(int old_node_id, int new_node_id);

public:
    // tree
    bool BuildTree(NodeHash& node_hash);
    bool InsertNode(int parent_id, Node* node);
    bool RemoveNode(int node_id, bool branch = false);
    bool DragNode(int destination_node_id, int node_id);
    bool InternalReference(int node_id) const;
    bool ExternalReference(int node_id) const;
    void LeafTotal(Node* node);

    // table
    void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id);
    void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list);
    bool InsertTransShadow(TransShadow* trans_shadow);
    TransShadow* AllocateTransShadow();

    bool RemoveTrans(int trans_id);
    bool UpdateTrans(int trans_id);
    bool UpdateCheckState(CString& column, CVariant& value, Check state);

    QHash<int, Trans*>* TransHash() { return &trans_hash_; } // 需要改变设计

    // common
    bool UpdateField(CString& table, CVariant& value, CString& field, int id);

protected:
    // tree
    virtual void ReadNode(Node* node, const QSqlQuery& query);
    virtual void WriteNode(Node* node, QSqlQuery& query);
    virtual void CalculateLeafTotal(Node* node, QSqlQuery& query);

    // QS means QueryString
    virtual QString BuildTreeQS() const = 0;
    virtual QString InsertNodeQS() const = 0;
    virtual QString RemoveNodeSecondQS() const = 0;
    virtual QString InternalReferenceQS() const = 0;
    virtual QString ExternalReferenceQS() const = 0;
    virtual QString LeafTotalQS() const = 0;

    QString RemoveNodeFirstQS() const;
    QString RemoveNodeBranchQS() const;
    QString RemoveNodeThirdQS() const;
    QString DragNodeFirstQS() const;
    QString DragNodeSecondQS() const;

    void BuildNodeHash(NodeHash& node_hash, QSqlQuery& query);
    bool DBTransaction(std::function<bool()> function);
    void ReadRelationship(const NodeHash& node_hash, QSqlQuery& query);
    void WriteRelationship(int node_id, int parent_id, QSqlQuery& query);

    // table
    virtual void ReadTrans(Trans* trans, const QSqlQuery& query);
    virtual void WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query);
    virtual void UpdateProductReference(int /*old_node_id*/, int /*new_node_id*/) { }
    virtual void QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query);
    virtual void ReplaceNode(int old_node_id, int new_node_id);

    virtual QString RRemoveNodeQS() const = 0;
    virtual QString RReplaceNodeQS() const = 0;
    virtual QString RUpdateProductReferenceQS() const = 0;
    virtual QString RelatedNodeTransQS() const = 0;
    virtual QString BuildTransShadowListQS() const = 0;
    virtual QString InsertTransShadowQS() const = 0;
    virtual QString BuildTransShadowListRangQS(CString& in_list) const = 0;

    QMultiHash<int, int> RelatedNodeTrans(int node_id) const;
    void ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left);

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

using SPSqlite = QSharedPointer<Sqlite>;

#endif // SQLITE_H
