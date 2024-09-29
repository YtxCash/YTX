#ifndef SQLITESALES_H
#define SQLITESALES_H

#include "sqlite.h"

class SqliteSales final : public Sqlite {
    Q_OBJECT

public:
    SqliteSales(CInfo& info, QObject* parent = nullptr);

protected:
    // tree
    void ReadNode(Node* node, const QSqlQuery& query) override;
    void WriteNode(Node* node, QSqlQuery& query) override;

    QString BuildTreeQS() const override;
    QString InsertNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString ExternalReferenceQS() const override { return QString(); }
    QString LeafTotalQS() const override { return QString(); }

    // table
    void WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query) override;
    void QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;
    void ReadTrans(Trans* trans, const QSqlQuery& query) override;
    void UpdateProductReference(int old_node_id, int new_node_id) override;
    void UpdateStakeholderReference(int old_node_id, int new_node_id) override;

    QString BuildTransShadowListQS() const override;
    QString InsertTransShadowQS() const override;
    QString BuildTransShadowListRangQS(CString& /*in_list*/) const override { return QString(); }
    QString RReplaceNodeQS() const override { return QString(); }
    QString RUpdateProductReferenceQS() const override;
    QString RUpdateStakeholderReferenceQS() const override;
};

#endif // SQLITESALES_H
