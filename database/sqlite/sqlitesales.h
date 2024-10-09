#ifndef SQLITESALES_H
#define SQLITESALES_H

#include "sqlite.h"

class SqliteSales final : public Sqlite {
    Q_OBJECT

public:
    SqliteSales(CInfo& info, QObject* parent = nullptr);

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) override;
    void WriteNodeBind(Node* node, QSqlQuery& query) override;

    QString ReadNodeQS() const override;
    QString WriteNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;

    // table
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) override;
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) override;
    void UpdateProductReference(int old_node_id, int new_node_id) override;
    void UpdateStakeholderReference(int old_node_id, int new_node_id) override;
    void UpdateTransBind(Trans* trans, QSqlQuery& query) override;

    QString ReadTransQS() const override;
    QString WriteTransQS() const override;
    QString RUpdateProductReferenceQS() const override;
    QString RUpdateStakeholderReferenceQS() const override;
    QString SearchTransQS() const override;
    QString UpdateTransQS() const override;
};

#endif // SQLITESALES_H
