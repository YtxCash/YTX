#include "searchsqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "global/sqlconnection.h"

SearchSqlite::SearchSqlite(CInfo& info, QHash<int, Trans*>* trans_hash)
    : db_ { SqlConnection::Instance().Allocate(info.section) }
    , trans_hash_ { trans_hash }
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

TransList SearchSqlite::QueryTransList(CString& text)
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
        return TransList();
    }

    Trans* trans {};
    TransList trans_list {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        if (trans_hash_->contains(id)) {
            trans = trans_hash_->value(id);
            trans_list.emplaceBack(trans);
            continue;
        }

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans->id = id;

        trans->lhs_node = query.value("lhs_node").toInt();
        trans->lhs_ratio = query.value("lhs_ratio").toDouble();
        trans->lhs_debit = query.value("lhs_debit").toDouble();
        trans->lhs_credit = query.value("lhs_credit").toDouble();

        trans->rhs_node = query.value("rhs_node").toInt();
        trans->rhs_ratio = query.value("rhs_ratio").toDouble();
        trans->rhs_debit = query.value("rhs_debit").toDouble();
        trans->rhs_credit = query.value("rhs_credit").toDouble();

        trans->code = query.value("code").toString();
        trans->description = query.value("description").toString();
        trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
        trans->date_time = query.value("date_time").toString();
        trans->state = query.value("state").toBool();

        trans_list.emplaceBack(trans);
    }

    return trans_list;
}
