#ifndef SQLITEORDER_H
#define SQLITEORDER_H

#include "sqlite.h"

class SqliteOrder final : public Sqlite {
    Q_OBJECT

public:
    SqliteOrder(CInfo& info, QObject* parent = nullptr);
    ~SqliteOrder();

    bool ReadNode(NodeHash& node_hash, const QDate& start_date, const QDate& end_date);

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;
    void WriteNodeBind(Node* node, QSqlQuery& query) const override;

    QString ReadNodeQS() const override;
    QString WriteNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;

    // table
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void UpdateProductReference(int old_node_id, int new_node_id) const override;
    void UpdateStakeholderReference(int old_node_id, int new_node_id) const override;
    void UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const override;

    QString UpdateNodeValueQS() const override;
    void UpdateNodeValueBind(const Node* node, QSqlQuery& query) const override;

    QString ReadTransQS() const override;
    QString WriteTransQS() const override;
    QString RUpdateProductReferenceQS() const override;
    QString RUpdateStakeholderReferenceQS() const override;
    QString SearchTransQS() const override;
    QString UpdateTransValueQS() const override;

private:
    void MoveToBuffer(NodeHash& node_hash, NodeHash& node_hash_buffer);

private:
    CString& node_;
    CString& transaction_;
    NodeHash node_hash_buffer_ {};
};

#endif // SQLITEORDER_H
