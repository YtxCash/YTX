#ifndef SQLITEPURCHASE_H
#define SQLITEPURCHASE_H

#include "sqlite.h"

class SqlitePurchase final : public Sqlite {
    Q_OBJECT

public:
    SqlitePurchase(CInfo& info, QObject* parent = nullptr);

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

    QString RRemoveNodeQS() const override;
    QString BuildTransShadowListQS() const override;
    QString InsertTransShadowQS() const override;
    QString BuildTransShadowListRangQS(CString& in_list) const override;
    QString RelatedNodeTransQS() const override { return QString(); }
    QString RReplaceNodeQS() const override { return QString(); }
    QString RUpdateProductReferenceQS() const override { return QString(); }
};

#endif // SQLITEPURCHASE_H
