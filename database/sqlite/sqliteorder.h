/*
 * Copyright (C) 2023 YtxCash
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SQLITEORDER_H
#define SQLITEORDER_H

#include "sqlite.h"

class SqliteOrder final : public Sqlite {
    Q_OBJECT

public:
    SqliteOrder(CInfo& info, QObject* parent = nullptr);
    ~SqliteOrder();

    bool ReadNode(NodeHash& node_hash, const QDate& start_date, const QDate& end_date);
    bool SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list);
    bool RetriveNode(NodeHash& node_hash, int node_id);

public slots:
    void RRemoveNode(int node_id, bool branch, bool is_helper) override;

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
    QString QSNodeTransToRemove() const override;

private:
    QString SearchNodeQS(CString& in_list) const;

private:
    CString& node_;
    CString& transaction_;
    NodeHash node_hash_buffer_ {};
};

#endif // SQLITEORDER_H
