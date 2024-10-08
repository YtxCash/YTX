#ifndef SQLITESTAKEHOLDER_H
#define SQLITESTAKEHOLDER_H

#include "sqlite.h"

class SqliteStakeholder final : public Sqlite {
    Q_OBJECT

public:
    SqliteStakeholder(CInfo& info, QObject* parent = nullptr);

public slots:
    bool RReplaceNode(int old_node_id, int new_node_id) override;
    bool RRemoveNode(int node_id) override;

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) override;
    void WriteNodeBind(Node* node, QSqlQuery& query) override;

    QString ReadNodeQS() const override;
    QString WriteNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString ExternalReferenceQS() const override;
    QString LeafTotalQS() const override { return {}; }

    // table
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) override;
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) override;
    void UpdateProductReference(int old_node_id, int new_node_id) override;
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;

    QString ReadTransRangeQS(CString& in_list) const override;
    QString ReadTransQS() const override;
    QString WriteTransQS() const override;
    QString RReplaceNodeQS() const override;
    QString RUpdateProductReferenceQS() const override;
    QString RUpdateStakeholderReferenceQS() const override { return {}; }
    QString UpdateTransQS() const override { return {}; }

private:
    QList<int> DialogReplaceNode(int old_node_id, int new_node_id);
};

#endif // SQLITESTAKEHOLDER_H
