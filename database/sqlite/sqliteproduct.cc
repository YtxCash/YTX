#include "sqliteproduct.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqliteProduct::SqliteProduct(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteProduct::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, type, unit, color, commission, unit_price, quantity, amount
    FROM product
    WHERE removed = 0
    )");
}

QString SqliteProduct::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO product (name, code, description, note, rule, type, unit, color, commission, unit_price)
    VALUES (:name, :code, :description, :note, :rule, :type, :unit, :color, :commission, :unit_price)
    )");
}

QString SqliteProduct::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        removed = 1
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteProduct::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM product_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteProduct::QSExternalReferencePS() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM stakeholder_transaction WHERE inside_product = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM sales_transaction WHERE inside_product = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM purchase_transaction WHERE inside_product = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteProduct::QSSupportReferenceFPTS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM product_transaction
    WHERE support_id = :support_id AND removed = 0
    )");
}

QString SqliteProduct::QSReplaceSupportTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        support_id = :new_node_id
    WHERE support_id = :old_node_id AND removed = 0
    )");
}

QString SqliteProduct::QSRemoveSupportFPTS() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        support_id = 0
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteProduct::QSFreeViewFPT() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM product_transaction
    WHERE ((lhs_node = :old_node_id AND rhs_node = :new_node_id) OR (rhs_node = :old_node_id AND lhs_node = :new_node_id)) AND removed = 0
    )");
}

QString SqliteProduct::QSSupportTransToMoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT id FROM product_transaction
    WHERE support_id = :support_id AND removed = 0
    )");
}

QString SqliteProduct::QSNodeTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT rhs_node, id FROM product_transaction
    WHERE lhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT lhs_node, id FROM product_transaction
    WHERE rhs_node = :node_id AND removed = 0
    )");
}

QString SqliteProduct::QSSupportTransToRemoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT support_id, id FROM product_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteProduct::QSLeafTotalFPT() const
{
    return QStringLiteral(R"(
    WITH node_balance AS (
        SELECT
            lhs_debit AS initial_debit,
            lhs_credit AS initial_credit,
            unit_cost * lhs_debit AS final_debit,
            unit_cost * lhs_credit AS final_credit
        FROM product_transaction
        WHERE lhs_node = :node_id AND removed = 0

        UNION ALL

        SELECT
            rhs_debit,
            rhs_credit,
            unit_cost * rhs_debit,
            unit_cost * rhs_credit
        FROM product_transaction
        WHERE rhs_node = :node_id AND removed = 0
    )
    SELECT
        SUM(initial_credit) - SUM(initial_debit) AS initial_balance,
        SUM(final_credit) - SUM(final_debit) AS final_balance
    FROM node_balance;
    )");
}

void SqliteProduct::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":rule", node->rule);
    query.bindValue(":type", node->type);
    query.bindValue(":unit", node->unit);
    query.bindValue(":color", node->color);
    query.bindValue(":commission", node->second);
    query.bindValue(":unit_price", node->first);
}

void SqliteProduct::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->rule = query.value("rule").toBool();
    node->type = query.value("type").toInt();
    node->unit = query.value("unit").toInt();
    node->color = query.value("color").toString();
    node->second = query.value("commission").toDouble();
    node->first = query.value("unit_price").toDouble();
    node->initial_total = query.value("quantity").toDouble();
    node->final_total = query.value("amount").toDouble();
}

void SqliteProduct::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->lhs_node = query.value("lhs_node").toInt();
    trans->lhs_debit = query.value("lhs_debit").toDouble();
    trans->lhs_credit = query.value("lhs_credit").toDouble();

    trans->rhs_node = query.value("rhs_node").toInt();
    trans->rhs_debit = query.value("rhs_debit").toDouble();
    trans->rhs_credit = query.value("rhs_credit").toDouble();

    trans->unit_price = query.value("unit_cost").toDouble();
    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->document = query.value("document").toString().split(kSemicolon, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
    trans->state = query.value("state").toBool();
    trans->support_id = query.value("support_id").toInt();
}

void SqliteProduct::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":unit_cost", *trans_shadow->unit_price);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":support_id", *trans_shadow->support_id);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":document", trans_shadow->document->join(kSemicolon));

    query.bindValue(":lhs_node", *trans_shadow->lhs_node);
    query.bindValue(":lhs_debit", *trans_shadow->lhs_debit);
    query.bindValue(":lhs_credit", *trans_shadow->lhs_credit);

    query.bindValue(":rhs_node", *trans_shadow->rhs_node);
    query.bindValue(":rhs_debit", *trans_shadow->rhs_debit);
    query.bindValue(":rhs_credit", *trans_shadow->rhs_credit);
}

void SqliteProduct::UpdateTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":lhs_node", *trans_shadow->lhs_node);
    query.bindValue(":lhs_debit", *trans_shadow->lhs_debit);
    query.bindValue(":lhs_credit", *trans_shadow->lhs_credit);
    query.bindValue(":rhs_node", *trans_shadow->rhs_node);
    query.bindValue(":rhs_debit", *trans_shadow->rhs_debit);
    query.bindValue(":rhs_credit", *trans_shadow->rhs_credit);
    query.bindValue(":trans_id", *trans_shadow->id);
}

QString SqliteProduct::QSReadNodeTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteProduct::QSUpdateNodeValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE product SET
        quantity = :quantity, amount = :amount
    WHERE id = :node_id
    )");
}

void SqliteProduct::UpdateNodeValueBindFPTO(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":quantity", node->initial_total);
    query.bindValue(":amount", node->final_total);
    query.bindValue(":node_id", node->id);
}

QString SqliteProduct::QSWriteNodeTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO product_transaction
    (date_time, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document)
    VALUES
    (:date_time, :lhs_node, :unit_cost, :lhs_debit, :lhs_credit, :rhs_node, :rhs_debit, :rhs_credit, :state, :description, :support_id, :code, :document)
    )");
}

QString SqliteProduct::QSReadTransRangeFPTS(CString& in_list) const
{
    return QString(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

QString SqliteProduct::QSReadSupportTransFPTS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteProduct::QSReplaceNodeTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        lhs_node = CASE WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id ELSE lhs_node END,
        rhs_node = CASE WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id ELSE rhs_node END
    WHERE lhs_node = :old_node_id OR rhs_node = :old_node_id;
    )");
}

QString SqliteProduct::QSUpdateTransValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        lhs_node = :lhs_node, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
        rhs_node = :rhs_node, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :trans_id
    )");
}

QString SqliteProduct::QSSearchTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE (lhs_debit = :text OR lhs_credit = :text OR rhs_debit = :text OR rhs_credit = :text OR description LIKE :description) AND removed = 0
    ORDER BY date_time
    )");
}
