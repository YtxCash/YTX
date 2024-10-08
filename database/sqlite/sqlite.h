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
    virtual bool RRemoveNode(int node_id);
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
    void LeafTotal(Node* node);
    QList<int> SearchNodeName(CString& text);

    // table
    void ReadTrans(TransShadowList& trans_shadow_list, int node_id);
    void ReadTransRange(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list);
    bool WriteTrans(TransShadow* trans_shadow);
    bool WriteTransRange(const QList<TransShadow*>& list);
    TransShadow* AllocateTransShadow();

    bool RemoveTrans(int trans_id);
    bool UpdateTrans(int trans_id);
    bool UpdateCheckState(CString& column, CVariant& value, Check state);
    void SearchTrans(TransList& trans_list, CString& text);

    // common
    bool UpdateField(CString& table, CVariant& value, CString& field, int id);

protected:
    // tree
    virtual void ReadNodeQuery(Node* node, const QSqlQuery& query);
    virtual void WriteNodeBind(Node* node, QSqlQuery& query);

    void CalculateLeafTotal(Node* node, QSqlQuery& query);

    // QS means QueryString
    virtual QString ReadNodeQS() const = 0;
    virtual QString WriteNodeQS() const = 0;
    virtual QString RemoveNodeSecondQS() const = 0;
    virtual QString InternalReferenceQS() const = 0;
    virtual QString ExternalReferenceQS() const = 0;
    virtual QString LeafTotalQS() const = 0;
    virtual QString UpdateTransQS() const = 0;
    virtual QString SearchTransQS() const = 0;

    QString RemoveNodeFirstQS() const;
    QString RemoveNodeBranchQS() const;
    QString RemoveNodeThirdQS() const;
    QString DragNodeFirstQS() const;
    QString DragNodeSecondQS() const;

    bool DBTransaction(std::function<bool()> function);
    void ReadRelationship(const NodeHash& node_hash, QSqlQuery& query);
    void WriteRelationship(int node_id, int parent_id, QSqlQuery& query);

    // table
    virtual void ReadTransQuery(Trans* trans, const QSqlQuery& query);
    virtual void UpdateTransBind(Trans* trans, QSqlQuery& query);
    virtual void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query);
    virtual void UpdateProductReference(int /*old_node_id*/, int /*new_node_id*/)
    {
        // finance, product, task
    }
    virtual void UpdateStakeholderReference(int /*old_node_id*/, int /*new_node_id*/)
    {
        // finance, product, task, stakeholder
    }
    virtual void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query);

    virtual QString RReplaceNodeQS() const = 0;
    virtual QString RUpdateProductReferenceQS() const = 0;
    virtual QString RUpdateStakeholderReferenceQS() const = 0;
    virtual QString ReadTransQS() const = 0;
    virtual QString WriteTransQS() const = 0;
    virtual QString ReadTransRangeQS(CString& in_list) const = 0;

    void ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left);

    QMultiHash<int, int> DialogReplaceNode(int old_node_id, int new_node_id);
    QMultiHash<int, int> DialogRemoveNode(int node_id);

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

using SPSqlite = QSharedPointer<Sqlite>;

#endif // SQLITE_H
