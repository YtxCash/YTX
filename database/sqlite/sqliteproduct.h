#ifndef SQLITEPRODUCT_H
#define SQLITEPRODUCT_H

#include "sqlite.h"

class SqliteProduct final : public Sqlite {
public:
    SqliteProduct(CInfo& info, QObject* parent = nullptr);

protected:
    // tree
    QString BuildTreeQS() const override;
    QString InsertNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString ExternalReferenceQS() const override;
    QString LeafTotalQS() const override;

    void WriteNode(Node* node, QSqlQuery& query) override;
    void ReadNode(Node* node, const QSqlQuery& query) override;

    void ReadTrans(Trans* trans, const QSqlQuery& query) override;
    void WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query) override;
    void UpdateTransBind(Trans* trans, QSqlQuery& query) override;

    QString BuildTransShadowListQS() const override;
    QString InsertTransShadowQS() const override;
    QString BuildTransShadowListRangQS(CString& in_list) const override;
    QString RReplaceNodeQS() const override;
    QString RUpdateProductReferenceQS() const override { return QString(); }
    QString RUpdateStakeholderReferenceQS() const override { return {}; }
    QString UpdateTransQS() const override;
};

#endif // SQLITEPRODUCT_H
