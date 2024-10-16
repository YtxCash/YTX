#ifndef SQLITEPRODUCT_H
#define SQLITEPRODUCT_H

#include "sqlite.h"

class SqliteProduct final : public Sqlite {
public:
    SqliteProduct(CInfo& info, QObject* parent = nullptr);

protected:
    // tree
    QString ReadNodeQS() const override;
    QString WriteNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString ExternalReferenceQS() const override;
    QString LeafTotalQS() const override;

    void WriteNodeBind(Node* node, QSqlQuery& query) const override;
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;

    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const override;

    QString UpdateNodeValueQS() const override;
    void UpdateNodeValueBind(const Node* node, QSqlQuery& query) const override;

    QString ReadTransQS() const override;
    QString WriteTransQS() const override;
    QString ReadTransRangeQS(CString& in_list) const override;
    QString RReplaceNodeQS() const override;
    QString UpdateTransValueQS() const override;
    QString SearchTransQS() const override;
};

#endif // SQLITEPRODUCT_H
