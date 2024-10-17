#include "sqlitestakeholder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteStakeholder::SqliteStakeholder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

bool SqliteStakeholder::RReplaceNode(int old_node_id, int new_node_id)
{
    // begin deal with trans hash
    auto node_trans { ReplaceNodeFunction(old_node_id, new_node_id) };
    auto trans_id_list { node_trans.values(old_node_id) };
    // end deal with trans hash

    // begin deal with database
    QSqlQuery query(*db_);
    CString& string { RReplaceNodeQS() };

    query.prepare(string);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Error in replace node setp" << query.lastError().text();
        return false;
    }
    // end deal with database

    if (!trans_id_list.isEmpty())
        emit SMoveMultiTrans(0, new_node_id, trans_id_list);

    emit SUpdateStakeholderReference(old_node_id, new_node_id);

    // SFreeView will mark all referenced transactions for removal. This must occur after SMoveMultiTrans()
    emit SFreeView(old_node_id);
    emit SRemoveNode(old_node_id);

    return true;
}

void SqliteStakeholder::RRemoveNode(int node_id)
{
    emit SFreeView(node_id);
    emit SRemoveNode(node_id);
    ReplaceNodeFunctionStakeholder(node_id, 0);
}

bool SqliteStakeholder::SearchPrice(TransShadow* order_trans_shadow, int party_id, int product_id, bool is_inside) const
{
    for (const auto* trans : trans_hash_) {
        if (is_inside && trans->node_id == party_id && trans->lhs_node == product_id) {
            *order_trans_shadow->unit_price = trans->unit_price;
            *order_trans_shadow->rhs_node = trans->rhs_node;
            return true;
        }

        if (!is_inside && trans->node_id == party_id && trans->rhs_node == product_id) {
            *order_trans_shadow->unit_price = trans->unit_price;
            *order_trans_shadow->lhs_node = trans->lhs_node;
            return true;
        }
    }

    return false;
}

bool SqliteStakeholder::UpdatePrice(int party_id, int inside_product_id, double value)
{
    // update unit_price
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash)
        if (trans->node_id == party_id && trans->lhs_node == inside_product_id) {
            trans->unit_price = value;
            UpdateField(info_.transaction, value, UNIT_PRICE, trans->id);
            return true;
        }

    // append unit_price in TableModelStakeholder
    auto* trans { ResourcePool<Trans>::Instance().Allocate() };
    auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

    trans->node_id = party_id;
    trans->lhs_node = inside_product_id;
    trans->unit_price = value;

    if (WriteTrans(trans)) {
        ConvertTrans(trans, trans_shadow, true);
        emit SAppendPrice(info_.section, trans_shadow);
        return true;
    }

    ResourcePool<Trans>::Instance().Recycle(trans);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return false;
}

bool SqliteStakeholder::ReadTrans(int node_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString& string { ReadTransQS() };
    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadTrans" << query.lastError().text();
        return false;
    }

    ReadTransFunction(query);
    return true;
}

QString SqliteStakeholder::ReadNodeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, employee, deadline, payment_period, tax_rate
    FROM stakeholder
    WHERE removed = 0
    )");
}

QString SqliteStakeholder::WriteNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder (name, code, description, note, rule, branch, unit, employee, deadline, payment_period, tax_rate)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :employee, :deadline, :payment_period, :tax_rate)
    )");
}

void SqliteStakeholder::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":rule", node->rule);
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
    UPDATE stakeholder_transaction SET
        removed = CASE WHEN node_id = :node_id THEN 1 ELSE removed END,
        outside_product = CASE WHEN outside_product = :node_id THEN 0 ELSE outside_product END
    WHERE node_id = :node_id OR outside_product = :node_id;
    )");
}

QString SqliteStakeholder::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM stakeholder_transaction
    WHERE (node_id = :node_id OR outside_product = :node_id) AND removed = 0
    )");
}

QString SqliteStakeholder::ExternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM sales WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM purchase WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM sales_transaction WHERE outside_product = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM purchase_transaction WHERE outside_product = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteStakeholder::ReadTransQS() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, node_id, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE node_id = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::WriteTransQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder_transaction
    (date_time, code, outside_product, node_id, unit_price, description, document, state, inside_product)
    VALUES
    (:date_time, :code, :outside_product, :node_id, :unit_price, :description, :document, :state, :inside_product)
    )");
}

QString SqliteStakeholder::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        node_id = CASE WHEN node_id = :old_node_id THEN :new_node_id ELSE node_id END,
        outside_product = CASE WHEN outside_product = :old_node_id THEN :new_node_id ELSE outside_product END
    WHERE node_id = :old_node_id OR outside_product = :old_node_id;
    )");
}

QString SqliteStakeholder::RUpdateProductReferenceQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )");
}

QString SqliteStakeholder::SearchTransQS() const
{
    return QStringLiteral(R"(
    SELECT  id, date_time, code, outside_product, node_id, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE (unit_price = :text OR description LIKE :description) AND removed = 0
    ORDER BY date_time
    )");
}

void SqliteStakeholder::ReadTransFunction(QSqlQuery& query)
{
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        if (trans_hash_.contains(id))
            continue;

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans->id = id;

        ReadTransQuery(trans, query);
        trans_hash_.insert(id, trans);
    }
}

bool SqliteStakeholder::WriteTrans(Trans* trans)
{
    QSqlQuery query(*db_);
    CString& string { WriteTransQS() };

    query.prepare(string);
    WriteTransBind(trans, query);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteTrans" << query.lastError().text();
        return false;
    }

    trans->id = query.lastInsertId().toInt();
    trans_hash_.insert(trans->id, trans);
    return true;
}

void SqliteStakeholder::WriteTransBind(Trans* trans, QSqlQuery& query) const
{
    query.bindValue(":date_time", trans->date_time);
    query.bindValue(":code", trans->code);
    query.bindValue(":node_id", trans->node_id);
    query.bindValue(":unit_price", trans->unit_price);
    query.bindValue(":description", trans->description);
    query.bindValue(":state", trans->state);
    query.bindValue(":document", trans->document.join(SEMICOLON));
    query.bindValue(":inside_product", trans->lhs_node);
    query.bindValue(":outside_product", trans->rhs_node);
}

void SqliteStakeholder::ReplaceNodeFunctionStakeholder(int old_node_id, int new_node_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
    }
}

QMultiHash<int, int> SqliteStakeholder::ReplaceNodeFunction(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };
    QMultiHash<int, int> hash {};

    for (auto* trans : const_trans_hash) {
        if (trans->node_id == old_node_id) {
            hash.emplace(old_node_id, trans->id);
            trans->node_id = new_node_id;
        }

        if (trans->rhs_node == old_node_id) {
            trans->rhs_node = new_node_id;
        }
    }

    return hash;
}

void SqliteStakeholder::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":node_id", *trans_shadow->node_id);
    query.bindValue(":unit_price", *trans_shadow->unit_price);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":inside_product", *trans_shadow->lhs_node);
    query.bindValue(":outside_product", *trans_shadow->rhs_node);
}

void SqliteStakeholder::UpdateProductReference(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash)
        if (trans->lhs_node == old_node_id)
            trans->lhs_node = new_node_id;
}

void SqliteStakeholder::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
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

QString SqliteStakeholder::ReadTransRangeQS(CString& in_list) const
{
    return QString(R"(
    SELECT id, date_time, code, outside_product, node_id, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

void SqliteStakeholder::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->rule = query.value("rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->employee = query.value("employee").toInt();
    node->party = query.value("deadline").toInt();
    node->first = query.value("payment_period").toInt();
    node->second = query.value("tax_rate").toDouble();
}

void SqliteStakeholder::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->node_id = query.value("node_id").toInt();
    trans->lhs_node = query.value("inside_product").toInt();
    trans->rhs_node = query.value("outside_product").toInt();
    trans->unit_price = query.value("unit_price").toDouble();
    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->state = query.value("state").toBool();
    trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
}
