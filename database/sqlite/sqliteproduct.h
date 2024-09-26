#ifndef SQLITEPRODUCT_H
#define SQLITEPRODUCT_H

#include "sqlite.h"

class SqliteProduct final : public Sqlite {
public:
    SqliteProduct(CInfo& info, QObject* parent = nullptr);

    // tree
    QString BuildTreeQueryString() const override;
    bool InsertNode(int parent_id, Node* node) override;
    bool ExternalReference(int node_id) const override;

private:
    // tree
    void ReadNode(Node* node, const QSqlQuery& query) override;
};

#endif // SQLITEPRODUCT_H
