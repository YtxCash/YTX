#include "sqlitestakeholder.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqliteStakeholder::SqliteStakeholder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteStakeholder::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, employee, deadline, payment_period, tax_rate
    FROM stakeholder
    WHERE removed = 0
    )");
}

QString SqliteStakeholder::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder (name, code, description, note, node_rule, branch, unit, employee, deadline, payment_period, tax_rate)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit, :employee, :deadline, :payment_period, :tax_rate)
    )");
}

void SqliteStakeholder::WriteNode(Node* node, QSqlQuery& query)
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":node_rule", node->node_rule);
    query.bindValue(":branch", node->branch);
    query.bindValue(":unit", node->unit);
    query.bindValue(":employee", node->employee);
    query.bindValue(":deadline", node->party);
    query.bindValue(":payment_period", node->first);
    query.bindValue(":tax_rate", node->second);
}

QString SqliteStakeholder::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET removed = 1
    WHERE lhs_node = :node_id
    )");
}

QString SqliteStakeholder::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM stakeholder_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::ExternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*)
    FROM (
        SELECT 1 FROM sales WHERE (party = :node_id OR employee = :node_id) AND removed = 0
        UNION ALL
        SELECT 1 FROM purchase WHERE (party = :node_id OR employee = :node_id) AND removed = 0
        UNION ALL
        SELECT 1 FROM sales_transaction WHERE rhs_node = :node_id AND removed = 0
        UNION ALL
        SELECT 1 FROM purchase_transaction WHERE rhs_node = :node_id AND removed = 0
    ) AS combined;
    )");
}

QString SqliteStakeholder::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node
    FROM stakeholder_transaction
    WHERE lhs_node = :lhs_node_id AND removed = 0
    )");
}

QString SqliteStakeholder::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder_transaction
    (date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node)
    VALUES
    (:date_time, :code, :lhs_node, :lhs_ratio, :description, :document, :state, :rhs_node)
    )");
}

QString SqliteStakeholder::RelatedNodeTransQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_node, id FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET lhs_node = :new_node_id
    WHERE lhs_node = :old_node_id
    )");
}

QString SqliteStakeholder::RUpdateProductReferenceQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET rhs_node = :new_node_id
    WHERE rhs_node = :old_node_id
    )");
}

void SqliteStakeholder::WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query)
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":lhs_node", *trans_shadow->node);
    query.bindValue(":lhs_ratio", *trans_shadow->ratio);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":rhs_node", *trans_shadow->related_node);
}

void SqliteStakeholder::UpdateProductReference(int old_node_id, int new_node_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto& trans : const_trans_hash)
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
}

QString SqliteStakeholder::RRemoveNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET removed = 1
    WHERE lhs_node = :node_id
    )");
}

QString SqliteStakeholder::BuildTransShadowListRangQS(QStringList& list) const
{
    return QString(R"(
    SELECT id, date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node
    FROM stakeholder_transaction
    WHERE id IN (%1) AND lhs_node = :node_id AND removed = 0
    )")
        .arg(list.join(", "));
}

void SqliteStakeholder::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->node_rule = query.value("node_rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->employee = query.value("employee").toInt();
    node->party = query.value("deadline").toInt();
    node->first = query.value("payment_period").toInt();
    node->second = query.value("tax_rate").toDouble();
}

void SqliteStakeholder::ReadTrans(Trans* trans, const QSqlQuery& query)
{
    trans->lhs_node = query.value("lhs_node").toInt();
    trans->rhs_node = query.value("rhs_node").toInt();
    trans->lhs_ratio = query.value("lhs_ratio").toDouble();
    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->state = query.value("state").toBool();
    trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
}
