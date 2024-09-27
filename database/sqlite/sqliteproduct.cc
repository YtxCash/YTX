#include "sqliteproduct.h"

#include <QSqlQuery>

SqliteProduct::SqliteProduct(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteProduct::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, commission, unit_price, initial_total, final_total
    FROM product
    WHERE removed = 0
    )");
}

QString SqliteProduct::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO product (name, code, description, note, node_rule, branch, unit, commission, unit_price)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit, :commission, :unit_price)
    )");
}

QString SqliteProduct::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqliteProduct::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM product_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteProduct::ExternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*)
    FROM (
        SELECT 1 FROM stakeholder_transaction WHERE rhs_node = :node_id AND removed = 0
        UNION ALL
        SELECT 1 FROM sales_transaction WHERE lhs_node = :node_id AND removed = 0
        UNION ALL
        SELECT 1 FROM purchase_transaction WHERE lhs_node = :node_id AND removed = 0
    ) AS combined;
    )");
}

QString SqliteProduct::LeafTotalQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_debit AS debit, lhs_credit AS credit, lhs_ratio AS ratio FROM product_transaction
    WHERE lhs_node = (:node_id) AND removed = 0
    UNION ALL
    SELECT rhs_debit, rhs_credit, rhs_ratio FROM product_transaction
    WHERE rhs_node = (:node_id) AND removed = 0
    )");
}

void SqliteProduct::WriteNode(Node* node, QSqlQuery& query)
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":node_rule", node->node_rule);
    query.bindValue(":branch", node->branch);
    query.bindValue(":unit", node->unit);
    query.bindValue(":commission", node->second);
    query.bindValue(":unit_price", node->discount);
}

void SqliteProduct::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->node_rule = query.value("node_rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->second = query.value("commission").toDouble();
    node->discount = query.value("unit_price").toDouble();
    node->initial_total = query.value("initial_total").toDouble();
    node->final_total = query.value("final_total").toDouble();
}

QString SqliteProduct::RRemoveNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqliteProduct::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM product_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteProduct::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO product_transaction
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
    )");
}

QString SqliteProduct::BuildTransShadowListRangQS(QStringList& list) const
{
    return QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM product_transaction
    WHERE id IN (%1) AND (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )")
        .arg(list.join(", "));
}

QString SqliteProduct::RelatedNodeTransQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_node, id FROM product_transaction
    WHERE rhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT rhs_node, id FROM product_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteProduct::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
    lhs_node = CASE
        WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id
        ELSE lhs_node
    END,
    rhs_node = CASE
        WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id
        ELSE rhs_node
    END
    )");
}
