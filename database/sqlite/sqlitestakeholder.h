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

#ifndef SQLITESTAKEHOLDER_H
#define SQLITESTAKEHOLDER_H

#include "sqlite.h"

class SqliteStakeholder final : public Sqlite {
    Q_OBJECT

public:
    SqliteStakeholder(CInfo& info, QObject* parent = nullptr);

signals:
    // send to signal station
    void SAppendPrice(Section section, TransShadow* trans_shadow);

public slots:
    void RReplaceNode(int old_node_id, int new_node_id) override;
    void RRemoveNode(int node_id) override;

public:
    bool SearchPrice(TransShadow* order_trans_shadow, int party_id, int product_id, bool is_inside) const;
    bool UpdatePrice(int party_id, int inside_product_id, CString& date_time, double value);
    bool ReadTrans(int node_id);

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;
    void WriteNodeBind(Node* node, QSqlQuery& query) const override;

    QString ReadNodeQS() const override;
    QString WriteNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString ExternalReferenceQS() const override;
    QString QSHelperReferenceFTS() const override;

    // table
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void UpdateProductReference(int old_node_id, int new_node_id) const override;
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;
    QMultiHash<int, int> ReplaceNodeFunction(int old_node_id, int new_node_id) const override;

    QString ReadTransRangeQS(CString& in_list) const override;
    QString ReadTransQS() const override;
    QString ReadTransHelperQS() const override;
    QString WriteTransQS() const override;
    QString RReplaceNodeQS() const override;
    QString RUpdateProductReferenceQS() const override;
    QString SearchTransQS() const override;
    QString RemoveNodeFirstQS() const override;

private:
    void ReadTransFunction(QSqlQuery& query);
    void WriteTransBind(Trans* trans, QSqlQuery& query) const;

    bool WriteTrans(Trans* trans);
    bool UpdateDateTimePrice(CString& date_time, double unit_price, int trans_id);

private:
    void ReplaceNodeFunctionStakeholder(int old_node_id, int new_node_id);
};

#endif // SQLITESTAKEHOLDER_H
