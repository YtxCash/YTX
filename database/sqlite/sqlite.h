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
    void SMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list);

    // send to tree model
    void SUpdateMultiLeafTotal(const QList<int>& node_id_list);
    void SRemoveNode(int node_id);
    // send to mainwindow
    void SFreeView(int node_id);
    // send to sql itsself
    void SUpdateProductReference(int old_node_id, int new_node_id);
    void SUpdateStakeholderReference(int old_node_id, int new_node_id);

public slots:
    // receive from remove node dialog
    virtual void RRemoveNode(int node_id);
    virtual bool RReplaceNode(int old_node_id, int new_node_id);
    // receive from sql
    bool RUpdateProductReference(int old_node_id, int new_node_id);
    bool RUpdateStakeholderReference(int old_node_id, int new_node_id);

public:
    // tree
    bool ReadNode(NodeHash& node_hash);
    bool WriteNode(int parent_id, Node* node);
    bool RemoveNode(int node_id, bool branch);
    bool DragNode(int destination_node_id, int node_id);
    bool InternalReference(int node_id) const;
    bool ExternalReference(int node_id) const;
    bool LeafTotal(Node* node);
    QList<int> SearchNodeName(CString& text);

    // table
    bool ReadTrans(TransShadowList& trans_shadow_list, int node_id);
    bool ReadTransRange(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list);
    bool WriteTrans(TransShadow* trans_shadow);
    bool WriteTransRange(const QList<TransShadow*>& list);
    TransShadow* AllocateTransShadow();

    bool RemoveTrans(int trans_id);
    bool UpdateTrans(int trans_id);
    bool UpdateCheckState(CString& column, CVariant& value, Check state);
    bool SearchTrans(TransList& trans_list, CString& text);

    // common
    bool UpdateField(CString& table, CVariant& value, CString& field, int id);

protected:
    // tree
    virtual void ReadNodeQuery(Node* node, const QSqlQuery& query);
    virtual void WriteNodeBind(Node* node, QSqlQuery& query);

    void CalculateLeafTotal(Node* node, QSqlQuery& query);
    bool DBTransaction(std::function<bool()> function);
    bool ReadRelationship(const NodeHash& node_hash, QSqlQuery& query);
    bool WriteRelationship(int node_id, int parent_id, QSqlQuery& query);

    // QS means QueryString
    virtual QString ReadNodeQS() const = 0;
    virtual QString WriteNodeQS() const = 0;
    virtual QString RemoveNodeSecondQS() const = 0;
    virtual QString InternalReferenceQS() const = 0;
    virtual QString SearchTransQS() const = 0;

    // default query string is empty
    virtual QString ExternalReferenceQS() const { return {}; }
    virtual QString LeafTotalQS() const { return {}; }
    virtual QString UpdateTransQS() const { return {}; }

    QString RemoveNodeFirstQS() const;
    QString RemoveNodeBranchQS() const;
    QString RemoveNodeThirdQS() const;
    QString DragNodeFirstQS() const;
    QString DragNodeSecondQS() const;

    // table
    virtual void ReadTransQuery(Trans* trans, const QSqlQuery& query);
    virtual void UpdateTransBind(Trans* trans, QSqlQuery& query);
    virtual void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query);
    virtual void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query);

    // default function do nothing
    virtual void UpdateProductReference(int /*old_node_id*/, int /*new_node_id*/) { }
    virtual void UpdateStakeholderReference(int /*old_node_id*/, int /*new_node_id*/) { }

    void ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left);
    QMultiHash<int, int> DialogReplaceNode(int old_node_id, int new_node_id);
    QMultiHash<int, int> DialogRemoveNode(int node_id);

    // default query string is empty
    virtual QString ReadTransQS() const = 0;
    virtual QString WriteTransQS() const = 0;

    virtual QString RUpdateProductReferenceQS() const { return {}; }
    virtual QString RUpdateStakeholderReferenceQS() const { return {}; }
    virtual QString RReplaceNodeQS() const { return {}; }
    virtual QString ReadTransRangeQS(CString& /*in_list*/) const { return {}; }

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

using SPSqlite = QSharedPointer<Sqlite>;

#endif // SQLITE_H
