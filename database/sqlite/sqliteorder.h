#ifndef SQLITEORDER_H
#define SQLITEORDER_H

#include "sqlite.h"

class SqliteOrder final : public Sqlite {
    Q_OBJECT

public:
    SqliteOrder(CInfo& info, QObject* parent = nullptr);

    // tree
    bool BuildTree(NodeHash& node_hash) override;
    bool InsertNode(int parent_id, Node* node) override;
    void NodeLeafTotal(Node* node) override;

    // table
    void BuildTransList(TransList& trans_list, int node_id) override;
    bool InsertTrans(Trans* trans) override;

    void BuildTransList(TransList& trans_list, int node_id, const QList<int>& trans_id_list) override;

private:
    // tree
    void BuildNodeHash(QSqlQuery& query, NodeHash& node_hash) override;

    // table
    void QueryTransList(TransList& trans_list, int node_id, QSqlQuery& query) override;
};

#endif // SQLITEORDER_H
