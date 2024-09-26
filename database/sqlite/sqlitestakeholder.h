#ifndef SQLITESTAKEHOLDER_H
#define SQLITESTAKEHOLDER_H

#include "sqlite.h"

class SqliteStakeholder final : public Sqlite {
    Q_OBJECT

public slots:
    // receive from remove node dialog
    bool RRemoveMulti(int node_id) override;
    bool RReplaceMulti(int old_node_id, int new_node_id) override;
    // receive from product sql
    bool RReplaceReferences(Section origin, int old_node_id, int new_node_id) override;

public:
    SqliteStakeholder(CInfo& info, QObject* parent = nullptr);

    // tree
    QString BuildTreeQueryString() const override;
    bool InsertNode(int parent_id, Node* node) override;
    bool ExternalReference(int node_id) const override;
    QString RemoveNodeQueryStringSecond() const override;
    QString InternalReferenceQueryString() const override;

    // table
    QString BuildTransShadowListQueryString() const override;
    bool InsertTransShadow(TransShadow* trans_shadow) override;
    QString BuildTransShadowListRangQueryString(QStringList& list) const override;

private:
    // tree
    void ReadNode(Node* node, const QSqlQuery& query) override;

    // table
    void ReadTrans(Trans* trans, const QSqlQuery& query) override;

    QList<int> TransID(int node_id, bool lhs = true);
};

#endif // SQLITESTAKEHOLDER_H
