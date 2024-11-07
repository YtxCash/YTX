#include "sqliteorder.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteOrder::SqliteOrder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
    , node_ { info.node }
    , transaction_ { info.transaction }
{
}

SqliteOrder::~SqliteOrder() { qDeleteAll(node_hash_buffer_); }

bool SqliteOrder::ReadNode(NodeHash& node_hash, const QDate& start_date, const QDate& end_date)
{
    CString& string { ReadNodeQS() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);

    query.bindValue(":start_date", start_date.toString(DATE_FST));
    query.bindValue(":end_date", end_date.toString(DATE_FST));

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNode" << query.lastError().text();
        return false;
    }

    if (!node_hash.isEmpty())
        node_hash.clear();

    Node* node {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        if (auto it = node_hash_buffer_.constFind(id); it != node_hash_buffer_.constEnd()) {
            it.value()->children.clear();
            it.value()->parent = nullptr;
            node_hash.insert(it.key(), it.value());
            continue;
        }

        node = ResourcePool<Node>::Instance().Allocate();
        ReadNodeQuery(node, query);
        node_hash.insert(id, node);
        node_hash_buffer_.insert(id, node);
    }

    if (!node_hash.isEmpty())
        ReadRelationship(node_hash, query);

    return true;
}

bool SqliteOrder::SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list)
{
    if (party_id_list.empty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const qsizetype batch_size { BATCH_SIZE };
    const auto total_batches { (party_id_list.size() + batch_size - 1) / batch_size };

    Node* node {};
    int id {};

    for (int batch_index = 0; batch_index != total_batches; ++batch_index) {
        int start = batch_index * batch_size;
        int end = std::min(start + batch_size, party_id_list.size());

        QList<int> current_batch { party_id_list.mid(start, end - start) };

        QStringList placeholder { current_batch.size(), "?" };
        QString string { SearchNodeQS(placeholder.join(",")) };

        query.prepare(string);

        for (int i = 0; i != current_batch.size(); ++i)
            query.bindValue(i, current_batch.at(i));

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in SearchNode, batch" << batch_index << ": " << query.lastError().text();
            continue;
        }

        while (query.next()) {
            id = query.value("id").toInt();

            if (auto it = node_hash_buffer_.constFind(id); it != node_hash_buffer_.constEnd()) {
                node_list.emplaceBack(it.value());
                continue;
            }

            node = ResourcePool<Node>::Instance().Allocate();
            ReadNodeQuery(node, query);
            node_list.emplaceBack(node);
            node_hash_buffer_.insert(id, node);
        }
    }

    return true;
}

bool SqliteOrder::RetriveNode(NodeHash& node_hash, int node_id)
{
    auto it = node_hash_buffer_.constFind(node_id);
    if (it != node_hash_buffer_.constEnd() && it.value())
        node_hash.insert(node_id, it.value());

    return true;
}

QString SqliteOrder::ReadNodeQS() const
{
    return QString(R"(
    SELECT name, id, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, finished, amount, settled
    FROM %1
    WHERE ((DATE(date_time) BETWEEN :start_date AND :end_date) OR branch = true) AND removed = 0
    )")
        .arg(node_);
}

QString SqliteOrder::WriteNodeQS() const
{
    return QString(R"(
    INSERT INTO %1 (name, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, finished, amount, settled)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :party, :employee, :date_time, :first, :second, :discount, :finished, :amount, :settled)
    )")
        .arg(node_);
}

QString SqliteOrder::RemoveNodeSecondQS() const
{
    return QString(R"(
    UPDATE %1 SET
        removed = 1
    WHERE node_id = :node_id
    )")
        .arg(transaction_);
}

QString SqliteOrder::InternalReferenceQS() const
{
    return QString(R"(
    SELECT COUNT(*) FROM %1
    WHERE node_id = :node_id AND removed = 0
    )")
        .arg(transaction_);
}

QString SqliteOrder::ReadTransQS() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, node_id, first, amount, discount, settled, outside_product, discount_price
    FROM %1
    WHERE node_id = :node_id AND removed = 0
    )")
        .arg(transaction_);
}

QString SqliteOrder::WriteTransQS() const
{
    return QString(R"(
    INSERT INTO %1 (code, inside_product, unit_price, second, description, node_id, first, amount, discount, settled, outside_product, discount_price)
    VALUES (:code, :inside_product, :unit_price, :second, :description, :node_id, :first, :amount, :discount, :settled, :outside_product, :discount_price)
    )")
        .arg(transaction_);
}

QString SqliteOrder::RUpdateProductReferenceQS() const
{
    return QString(R"(
    UPDATE %1 SET
        inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )")
        .arg(transaction_);
}

QString SqliteOrder::RUpdateStakeholderReferenceQS() const
{
    return QString(R"(
    BEGIN TRANSACTION;

    -- Update the outside_product in transaction table
    UPDATE %2 SET
        outside_product = :new_node_id
    WHERE outside_product = :old_node_id;

    -- Update the party and employee in node table
    UPDATE %1 SET
        party = CASE WHEN party = :old_node_id THEN :new_node_id ELSE party END,
        employee = CASE WHEN employee = :old_node_id THEN :new_node_id ELSE employee END
    WHERE party = :old_node_id OR employee = :old_node_id;

    COMMIT;
    )")
        .arg(node_, transaction_);
}

QString SqliteOrder::SearchTransQS() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, node_id, first, amount, discount, settled, outside_product, discount_price
    FROM %1
    WHERE (first = :text OR second = :text OR description LIKE :description) AND removed = 0
    )")
        .arg(transaction_);
}

QString SqliteOrder::UpdateTransValueQS() const
{
    return QString(R"(
    UPDATE %1 SET
        second = :second, amount = :amount, discount = :discount, settled = :settled
    WHERE id = :trans_id
    )")
        .arg(transaction_);
}

QString SqliteOrder::SearchNodeQS(CString& in_list) const
{
    return QString(R"(
    SELECT name, id, code, description, note, rule, branch, unit, party, employee, date_time, first, second, discount, finished, amount, settled
    FROM %1
    WHERE party IN (%2) AND removed = 0
    )")
        .arg(node_, in_list);
}

void SqliteOrder::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
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

void SqliteOrder::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
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

void SqliteOrder::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
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

void SqliteOrder::UpdateProductReference(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id)
            trans->lhs_node = new_node_id;
    }
}

void SqliteOrder::UpdateStakeholderReference(int old_node_id, int new_node_id) const
{
    // for party's product reference
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
    }
}

void SqliteOrder::UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":second", *trans_shadow->lhs_credit);
    query.bindValue(":amount", *trans_shadow->rhs_credit);
    query.bindValue(":discount", *trans_shadow->rhs_debit);
    query.bindValue(":settled", *trans_shadow->settled);
    query.bindValue(":trans_id", *trans_shadow->id);
}

QString SqliteOrder::UpdateNodeValueQS() const
{
    return QString(R"(
    UPDATE %1 SET
        amount = :amount, settled = :settled, second = :second, discount = :discount
    WHERE id = :node_id
    )")
        .arg(node_);
}

void SqliteOrder::UpdateNodeValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":amount", node->initial_total);
    query.bindValue(":second", node->second);
    query.bindValue(":discount", node->discount);
    query.bindValue(":settled", node->final_total);
    query.bindValue(":node_id", node->id);
}

void SqliteOrder::ReadNodeQuery(Node* node, const QSqlQuery& query) const
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
    node->finished = query.value("finished").toBool();
    node->initial_total = query.value("amount").toDouble();
    node->final_total = query.value("settled").toDouble();
}

void SqliteOrder::WriteNodeBind(Node* node, QSqlQuery& query) const
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
    query.bindValue(":finished", node->finished);
    query.bindValue(":amount", node->initial_total);
    query.bindValue(":settled", node->final_total);
}
