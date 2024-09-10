#ifndef SQLITEPRODUCT_H
#define SQLITEPRODUCT_H

#include "sqlite.h"

class SqliteProduct final : public Sqlite {
public:
    SqliteProduct(CInfo& info, QObject* parent = nullptr);

    // tree
    bool BuildTree(NodeHash& node_hash) override;
    bool InsertNode(int parent_id, Node* node) override;
    bool NodeExternalReferences(int node_id) const override;
    bool UpdateNodeSimple(const Node* node) override;

private:
    // tree
    void BuildNodeHash(QSqlQuery& query, NodeHash& node_hash) override;
};

#endif // SQLITEPRODUCT_H
