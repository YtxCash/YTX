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
    bool BuildTree(NodeHash& node_hash) override;
    bool InsertNode(int parent_id, Node* node) override;
    bool NodeInternalReferences(int node_id) const override;
    bool NodeExternalReferences(int node_id) const override;
    bool UpdateNodeSimple(const Node* node) override;
    bool RemoveNode(int node_id, bool branch = false) override;

    // table
    void BuildTransShadowList(TransShadowList& trans_shadow_list, int outside_id) override;
    bool InsertTransShadow(TransShadow* trans_shadow) override;

    void BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list) override;

private:
    // tree
    void BuildNodeHash(QSqlQuery& query, NodeHash& node_hash) override;

    // table
    void QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;

    QList<int> TransID(int node_id, bool lhs = true);
};

#endif // SQLITESTAKEHOLDER_H