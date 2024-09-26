#ifndef SQLITEORDER_H
#define SQLITEORDER_H

#include "sqlite.h"

class SqliteOrder final : public Sqlite {
    Q_OBJECT

public:
    SqliteOrder(CInfo& info, QObject* parent = nullptr);

    // tree
    QString BuildTreeQueryString() const override;

    bool InsertNode(int parent_id, Node* node) override;
    void NodeLeafTotal(Node* node) override;

    // table
    QString BuildTransShadowListQueryString() const override;
    bool InsertTransShadow(TransShadow* trans_shadow) override;

private:
    // tree
    void ReadNode(Node* node, const QSqlQuery& query) override;
};

#endif // SQLITEORDER_H
