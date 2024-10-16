#include "sqlitepurchase.h"

#include <QSqlQuery>

#include "global/resourcepool.h"

SqlitePurchase::SqlitePurchase(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqlitePurchase::ReadNodeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, locked, amount, settled
    FROM purchase
    WHERE removed = 0
    )");
}

QString SqlitePurchase::WriteNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO purchase (name, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, locked, amount, settled)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :party, :employee, :date_time, :first, :second, :discount, :locked, :amount, :settled)
    )");
}

QString SqlitePurchase::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE purchase_transaction SET
        removed = 1
    WHERE node_id = :node_id
    )");
}

QString SqlitePurchase::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM purchase_transaction
    WHERE node_id = :node_id AND removed = 0
    )");
}

QString SqlitePurchase::ReadTransQS() const
{
    return QStringLiteral(R"(
    SELECT id, code, inside_product, unit_price, second, description, node_id, first, amount, discount, settled, outside_product, discount_price
    FROM purchase_transaction
    WHERE node_id = :node_id AND removed = 0
    )");
}

QString SqlitePurchase::WriteTransQS() const
{
    return QStringLiteral(R"(
    INSERT INTO purchase_transaction (code, inside_product, unit_price, second, description, node_id, first, amount, discount, settled, outside_product, discount_price)
    VALUES (:code, :inside_product, :unit_price, :second, :description, :node_id, :first, :amount, :discount, :settled, :outside_product, :discount_price)
    )");
}

QString SqlitePurchase::RUpdateProductReferenceQS() const
{
    return QStringLiteral(R"(
    UPDATE purchase_transaction SET
        inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )");
}

QString SqlitePurchase::RUpdateStakeholderReferenceQS() const
{
    return QStringLiteral(R"(
    BEGIN TRANSACTION;

    -- Update the outside_product in purchase_transaction table
    UPDATE purchase_transaction SET
        outside_product = :new_node_id
    WHERE outside_product = :old_node_id;

    -- Update the party and employee in purchase table
    UPDATE purchase SET
        party = CASE WHEN party = :old_node_id THEN :new_node_id ELSE party END,
        employee = CASE WHEN employee = :old_node_id THEN :new_node_id ELSE employee END
    WHERE party = :old_node_id OR employee = :old_node_id;

    COMMIT;
    )");
}

QString SqlitePurchase::SearchTransQS() const
{
    return QStringLiteral(R"(
    SELECT id, code, inside_product, unit_price, second, description, node_id, first, amount, discount, settled, outside_product, discount_price
    FROM purchase_transaction
    WHERE (first = :text OR second = :text OR description LIKE :description) AND removed = 0
    )");
}

QString SqlitePurchase::UpdateTransValueQS() const
{
    return QStringLiteral(R"(
    UPDATE purchase_transaction SET
        second = :second, amount = :amount, discount = :discount, settled = :settled
    WHERE id = :trans_id
    )");
}

void SqlitePurchase::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":inside_product", *trans_shadow->lhs_node);
    query.bindValue(":unit_price", *trans_shadow->unit_price);
    query.bindValue(":second", *trans_shadow->lhs_credit);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":node_id", *trans_shadow->node_id);
    query.bindValue(":first", *trans_shadow->lhs_debit);
    query.bindValue(":amount", *trans_shadow->rhs_credit);
    query.bindValue(":discount", *trans_shadow->rhs_debit);
    query.bindValue(":settled", *trans_shadow->settled);
    query.bindValue(":outside_product", *trans_shadow->rhs_node);
    query.bindValue(":discount_price", *trans_shadow->discount_price);
    query.bindValue(":description", *trans_shadow->description);
}

void SqlitePurchase::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->code = query.value("code").toString();
    trans->lhs_node = query.value("inside_product").toInt();
    trans->unit_price = query.value("unit_price").toDouble();
    trans->lhs_credit = query.value("second").toDouble();
    trans->description = query.value("description").toString();
    trans->node_id = query.value("node_id").toInt();
    trans->lhs_debit = query.value("first").toInt();
    trans->rhs_credit = query.value("amount").toDouble();
    trans->settled = query.value("settled").toDouble();
    trans->rhs_debit = query.value("discount").toDouble();
    trans->rhs_node = query.value("outside_product").toInt();
    trans->discount_price = query.value("discount_price").toDouble();
    trans->description = query.value("description").toString();
}

void SqlitePurchase::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        trans->id = id;

        ReadTransQuery(trans, query);
        trans_hash_.insert(id, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

void SqlitePurchase::UpdateProductReference(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id)
            trans->lhs_node = new_node_id;
    }
}

void SqlitePurchase::UpdateStakeholderReference(int old_node_id, int new_node_id) const
{
    // for party's product reference
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
    }
}

void SqlitePurchase::UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":second", *trans_shadow->lhs_credit);
    query.bindValue(":amount", *trans_shadow->rhs_credit);
    query.bindValue(":discount", *trans_shadow->rhs_debit);
    query.bindValue(":settled", *trans_shadow->settled);
    query.bindValue(":trans_id", *trans_shadow->id);
}

QString SqlitePurchase::UpdateNodeValueQS() const
{
    return QStringLiteral(R"(
    UPDATE purchase SET
        amount = :amount, settled = :settled, second = :second, discount = :discount
    WHERE id = :node_id
    )");
}

void SqlitePurchase::UpdateNodeValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":amount", node->initial_total);
    query.bindValue(":second", node->second);
    query.bindValue(":discount", node->discount);
    query.bindValue(":settled", node->final_total);
    query.bindValue(":node_id", node->id);
}

void SqlitePurchase::ReadNodeQuery(Node* node, const QSqlQuery& query) const
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
    node->initial_total = query.value("amount").toDouble();
    node->final_total = query.value("settled").toDouble();
}

void SqlitePurchase::WriteNodeBind(Node* node, QSqlQuery& query) const
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
    query.bindValue(":amount", node->initial_total);
    query.bindValue(":settled", node->final_total);
}
