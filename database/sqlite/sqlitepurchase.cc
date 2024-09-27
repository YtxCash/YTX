#include "sqlitepurchase.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqlitePurchase::SqlitePurchase(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqlitePurchase::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total
    FROM purchase
    WHERE removed = 0
    )");
}

QString SqlitePurchase::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO purchase (name, code, description, note, node_rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit, :party, :employee, :date_time, :first, :second, :discount, :locked, :initial_total, :final_total)
    )");
}

QString SqlitePurchase::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE purchase_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqlitePurchase::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM purchase_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

void SqlitePurchase::WriteNode(Node* node, QSqlQuery& query)
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":node_rule", node->node_rule);
    query.bindValue(":branch", node->branch);
    query.bindValue(":unit", node->unit);
    query.bindValue(":party", node->party);
    query.bindValue(":employee", node->employee);
    query.bindValue(":date_time", node->date_time);
    query.bindValue(":first", node->first);
    query.bindValue(":second", node->second);
    query.bindValue(":discount", node->discount);
    query.bindValue(":locked", node->locked);
    query.bindValue(":initial_total", node->initial_total);
    query.bindValue(":final_total", node->final_total);
}

QString SqlitePurchase::RRemoveNodeQS() const { return QString(); }

QString SqlitePurchase::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM purchase_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqlitePurchase::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO purchase_transaction (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
    )");
}

void SqlitePurchase::WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query)
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":lhs_node", *trans_shadow->node);
    query.bindValue(":lhs_ratio", *trans_shadow->ratio);
    query.bindValue(":lhs_debit", *trans_shadow->debit);
    query.bindValue(":lhs_credit", *trans_shadow->credit);
    query.bindValue(":rhs_node", *trans_shadow->related_node);
    query.bindValue(":rhs_ratio", *trans_shadow->related_ratio);
    query.bindValue(":rhs_debit", *trans_shadow->related_debit);
    query.bindValue(":rhs_credit", *trans_shadow->related_credit);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
}

QString SqlitePurchase::BuildTransShadowListRangQS(QStringList& list) const
{
    return QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM purchase_transaction
    WHERE id IN (%1) AND (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )")
        .arg(list.join(", "));
}

void SqlitePurchase::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->node_rule = query.value("node_rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->party = query.value("party").toInt();
    node->employee = query.value("employee").toInt();
    node->date_time = query.value("date_time").toString();
    node->first = query.value("first").toInt();
    node->second = query.value("second").toDouble();
    node->discount = query.value("discount").toDouble();
    node->locked = query.value("locked").toBool();
    node->initial_total = query.value("initial_total").toDouble();
    node->final_total = query.value("final_total").toDouble();
}
