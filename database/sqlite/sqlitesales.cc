#include "sqlitesales.h"

#include <QSqlQuery>

#include "global/resourcepool.h"

SqliteSales::SqliteSales(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteSales::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total
    FROM sales
    WHERE removed = 0
    )");
}

QString SqliteSales::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO sales (name, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :party, :employee, :date_time, :first, :second, :discount, :locked, :initial_total, :final_total)
    )");
}

QString SqliteSales::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE sales_transaction
    SET removed = 1
    WHERE node_id = :node_id
    )");
}

QString SqliteSales::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM sales_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

void SqliteSales::WriteNode(Node* node, QSqlQuery& query)
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":rule", node->rule);
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

QString SqliteSales::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, code, lhs_node, lhs_ratio, second, node_id, first, initial_subtotal, discount, rhs_node, rhs_ratio
    FROM sales_transaction
    WHERE node_id = :node_id AND removed = 0
    )");
}

QString SqliteSales::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO sales_transaction (code, lhs_node, lhs_ratio, second, node_id, first, initial_subtotal, discount, rhs_node, rhs_ratio)
    VALUES (:code, :lhs_node, :lhs_ratio, :second, :node_id, :first, :initial_subtotal, :discount, :rhs_node, :rhs_ratio)
    )");
}

QString SqliteSales::RUpdateProductReferenceQS() const
{
    return QStringLiteral(R"(
    UPDATE sales_transaction
    SET lhs_node = :new_node_id
    WHERE lhs_node = :old_node_id
    )");
}

QString SqliteSales::RUpdateStakeholderReferenceQS() const
{
    return QStringLiteral(R"(
    BEGIN TRANSACTION;

    -- Update the rhs_node in sales_transaction table
    UPDATE sales_transaction
    SET rhs_node = :new_node_id
    WHERE rhs_node = :old_node_id;

    -- Update the party and employee in sales table
    UPDATE sales
    SET party = CASE WHEN party = :old_node_id THEN :new_node_id ELSE party END,
        employee = CASE WHEN employee = :old_node_id THEN :new_node_id ELSE employee END
    WHERE party = :old_node_id OR employee = :old_node_id;

    COMMIT;
    )");
}

void SqliteSales::WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query)
{
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":lhs_node", *trans_shadow->node);
    query.bindValue(":lhs_ratio", *trans_shadow->ratio);
    query.bindValue(":second", *trans_shadow->credit);
    query.bindValue(":node_id", *trans_shadow->node_id);
    query.bindValue(":first", *trans_shadow->debit);
    query.bindValue(":initial_subtotal", *trans_shadow->related_credit);
    query.bindValue(":discount", *trans_shadow->related_debit);
    query.bindValue(":rhs_node", *trans_shadow->related_node);
    query.bindValue(":rhs_ratio", *trans_shadow->related_ratio);
}

void SqliteSales::ReadTrans(Trans* trans, const QSqlQuery& query)
{
    trans->code = query.value("code").toString();
    trans->lhs_node = query.value("lhs_node").toInt();
    trans->lhs_ratio = query.value("lhs_ratio").toDouble();
    trans->lhs_credit = query.value("second").toDouble();
    trans->node_id = query.value("node_id").toInt();
    trans->lhs_debit = query.value("first").toInt();
    trans->rhs_credit = query.value("initial_subtotal").toDouble();
    trans->rhs_debit = query.value("discount").toDouble();
    trans->rhs_node = query.value("rhs_node").toInt();
    trans->rhs_ratio = query.value("rhs_ratio").toDouble();
}

void SqliteSales::UpdateProductReference(int old_node_id, int new_node_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id)
            trans->lhs_node = new_node_id;
    }
}

void SqliteSales::UpdateStakeholderReference(int old_node_id, int new_node_id)
{
    // for party's product reference
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
    }
}

void SqliteSales::QueryTransShadowList(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        trans->id = id;

        ReadTrans(trans, query);
        trans_hash_.insert(id, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

void SqliteSales::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->rule = query.value("rule").toBool();
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
