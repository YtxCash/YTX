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
    void MoveToBuffer(QHash<int, Node*>& node_hash, QHash<int, Node*>& node_hash_buffer, const QSet<int>& keep)
    {
        // 预分配空间以提高性能
        const auto estimated_moves { node_hash.size() - keep.size() };
        if (estimated_moves >= 1) {
            node_hash_buffer.reserve(node_hash_buffer.size() + estimated_moves);
        }

        for (auto it = node_hash.begin(); it != node_hash.end();) {
            if (!keep.contains(it.key())) {
                node_hash_buffer.insert(it.key(), it.value());
                it = node_hash.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    CString& node_;
    CString& transaction_;
    NodeHash node_hash_buffer_ {};
};

#endif // SQLITEORDER_H
