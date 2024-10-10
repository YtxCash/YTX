#ifndef SQLITEPURCHASE_H
#define SQLITEPURCHASE_H

#include "sqlite.h"

class SqlitePurchase final : public Sqlite {
    Q_OBJECT

public:
    SqlitePurchase(CInfo& info, QObject* parent = nullptr);

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
    void UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) override; // for UpdateSecond

    QString UpdateNodeValueQS() const override;
    void UpdateNodeValueBind(const Node* node, QSqlQuery& query) override;

    QString ReadTransQS() const override;
    QString WriteTransQS() const override;
    QString RUpdateProductReferenceQS() const override;
    QString RUpdateStakeholderReferenceQS() const override;
    QString SearchTransQS() const override;
    QString UpdateTransValueQS() const override; // for UpdateSecond
};

#endif // SQLITEPURCHASE_H
