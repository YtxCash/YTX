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
