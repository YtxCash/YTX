#include "searchsqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "global/sqlconnection.h"

SearchSqlite::SearchSqlite(CInfo& info, QHash<int, Transaction*>* transaction_hash)
    : db_ { SqlConnection::Instance().Allocate(info.section) }
    , transaction_hash_ { transaction_hash }
    , info_ { info }
{
}

QList<int> SearchSqlite::Node(CString& text)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString("SELECT id "
                        "FROM %1 "
                        "WHERE removed = 0 AND name LIKE '%%2%' ")
                    .arg(info_.node, text);

    if (text.isEmpty())
        part = QString("SELECT id "
                       "FROM %1 "
                       "WHERE removed = 0 ")
                   .arg(info_.node);

    query.prepare(part);
    query.bindValue(":text", text);
    if (!query.exec()) {
        qWarning() << "Error in Construct Search Node Table" << query.lastError().text();
        return QList<int>();
    }

    int node_id {};
    QList<int> node_list {};

    while (query.next()) {
        node_id = query.value("id").toInt();
        node_list.emplaceBack(node_id);
    }

    return node_list;
}

TransactionList SearchSqlite::Trans(CString& text)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString("SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, transport, location, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, "
                        "description, code, document, date_time "
                        "FROM %1 "
                        "WHERE (lhs_debit = :text OR lhs_credit = :text OR rhs_debit = :text OR rhs_credit = :text OR description LIKE '%%2%') AND removed = 0 "
                        "ORDER BY date_time ")
                    .arg(info_.transaction, text);

    query.prepare(part);
    query.bindValue(":text", text);
    if (!query.exec()) {
        qWarning() << "Error in Construct Search Transaction model" << query.lastError().text();
        return TransactionList();
    }

    Transaction* transaction {};
    TransactionList transaction_list {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        if (transaction_hash_->contains(id)) {
            transaction = transaction_hash_->value(id);
            transaction_list.emplaceBack(transaction);
            continue;
        }

        transaction = ResourcePool<Transaction>::Instance().Allocate();
        transaction->id = id;

        transaction->lhs_node = query.value("lhs_node").toInt();
        transaction->lhs_ratio = query.value("lhs_ratio").toDouble();
        transaction->lhs_debit = query.value("lhs_debit").toDouble();
        transaction->lhs_credit = query.value("lhs_credit").toDouble();

        transaction->rhs_node = query.value("rhs_node").toInt();
        transaction->rhs_ratio = query.value("rhs_ratio").toDouble();
        transaction->rhs_debit = query.value("rhs_debit").toDouble();
        transaction->rhs_credit = query.value("rhs_credit").toDouble();

        transaction->code = query.value("code").toString();
        transaction->description = query.value("description").toString();
        transaction->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
        transaction->date_time = query.value("date_time").toString();
        transaction->state = query.value("state").toBool();

        transaction_list.emplaceBack(transaction);
    }

    return transaction_list;
}
