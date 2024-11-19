#include "sqlitestakeholder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteStakeholder::SqliteStakeholder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

void SqliteStakeholder::RReplaceNode(int old_node_id, int new_node_id, bool is_helper)
{
    QList<int> helper_trans {};
    if (is_helper)
        helper_trans = HelperTransToMoveFPTS(old_node_id);

    emit SFreeView(old_node_id);
    emit SRemoveNode(old_node_id);
    emit SUpdateStakeholder(old_node_id, new_node_id);

    if (!is_helper) {
        RemoveNode(old_node_id, false, is_helper);
        return;
    }

    // begin deal with database
    QSqlQuery query(*db_);
    CString& string { QSReplaceHelperTransFPTS() };

    query.prepare(string);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Error in RReplaceNode" << query.lastError().text();
        return;
    }
    // end deal with database

    ReplaceHelperFunction(old_node_id, new_node_id);
    RemoveNode(old_node_id, false, is_helper);
    emit SMoveMultiHelperTransFPTS(info_.section, new_node_id, helper_trans);
}

void SqliteStakeholder::RRemoveNode(int node_id, bool branch, bool is_helper)
{
    emit SFreeView(node_id);
    emit SRemoveNode(node_id);
    emit SUpdateStakeholder(node_id, 0);

    const QMultiHash<int, int> node_trans { TransToRemove(node_id, false) };
    const QMultiHash<int, int> helper_trans { TransToRemove(node_id, true) };

    RemoveNode(node_id, branch, is_helper);

    if (is_helper) {
        RemoveHelperFunction(node_id);
        return;
    }

    if (!helper_trans.isEmpty())
        emit SRemoveMultiTrans(helper_trans);

    // Recycle trans resources
    const auto trans { node_trans.values() };

    for (int trans_id : trans)
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
}

bool SqliteStakeholder::SearchPrice(TransShadow* order_trans_shadow, int party_id, int product_id, bool is_inside) const
{
    for (const auto* trans : trans_hash_) {
        if (is_inside && trans->lhs_node == party_id && trans->rhs_node == product_id) {
            *order_trans_shadow->unit_price = trans->unit_price;
            *order_trans_shadow->helper_node = trans->helper_node;
            return true;
        }

        if (!is_inside && trans->lhs_node == party_id && trans->helper_node == product_id) {
            *order_trans_shadow->unit_price = trans->unit_price;
            *order_trans_shadow->rhs_node = trans->rhs_node;
            return true;
        }
    }

    return false;
}

bool SqliteStakeholder::UpdatePrice(int party_id, int inside_product_id, CString& date_time, double value)
{
    // update unit_price
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash)
        if (trans->lhs_node == party_id && trans->rhs_node == inside_product_id) {
            trans->unit_price = value;
            trans->date_time = date_time;
            UpdateDateTimePrice(date_time, value, trans->id);
            return true;
        }

    // append unit_price in TableModelStakeholder
    auto* trans { ResourcePool<Trans>::Instance().Allocate() };
    auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

    trans->lhs_node = party_id;
    trans->rhs_node = inside_product_id;
    trans->unit_price = value;
    trans->date_time = date_time;

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

    CString& string { QSReadNodeTrans() };
    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadTrans" << query.lastError().text();
        return false;
    }

    ReadTransFunction(query);
    return true;
}

QString SqliteStakeholder::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, is_helper, employee, deadline, payment_period, tax_rate
    FROM stakeholder
    WHERE removed = 0
    )");
}

QString SqliteStakeholder::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder (name, code, description, note, rule, branch, unit, is_helper, employee, deadline, payment_period, tax_rate)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :is_helper, :employee, :deadline, :payment_period, :tax_rate)
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
    query.bindValue(":is_helper", node->is_helper);
    query.bindValue(":employee", node->employee);
    query.bindValue(":deadline", node->date_time);
    query.bindValue(":payment_period", node->first);
    query.bindValue(":tax_rate", node->second);
}

QString SqliteStakeholder::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        removed = 1
    WHERE lhs_node = :node_id;
    )");
}

QString SqliteStakeholder::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM stakeholder_transaction WHERE lhs_node = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM stakeholder WHERE employee = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteStakeholder::QSExternalReferencePS() const
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

QString SqliteStakeholder::QSHelperReferenceFPTS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM stakeholder_transaction
    WHERE outside_product = :helper_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSReplaceHelperTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        outside_product = :new_node_id
    WHERE outside_product = :old_node_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSRemoveHelperFPTS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        outside_product = 0
    WHERE outside_product = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSHelperTransToMoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT id FROM stakeholder_transaction
    WHERE outside_product = :helper_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSHelperTransToRemoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT outside_product, id FROM stakeholder_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSReadNodeTrans() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSReadHelperTransFPTS() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE outside_product = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSWriteNodeTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder_transaction
    (date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product)
    VALUES
    (:date_time, :code, :outside_product, :lhs_node, :unit_price, :description, :document, :state, :inside_product)
    )");
}

QString SqliteStakeholder::QSReplaceNodeTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        lhs_node = :new_node_id
    WHERE lhs_node = :old_node_id AND removed = 0
    )");
}

QString SqliteStakeholder::QSUpdateProductReferenceSO() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )");
}

QString SqliteStakeholder::QSSearchTrans() const
{
    return QStringLiteral(R"(
    SELECT  id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE (unit_price = :text OR description LIKE :description) AND removed = 0
    ORDER BY date_time
    )");
}

QString SqliteStakeholder::QSRemoveNodeFirst() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder
    SET
        removed = CASE WHEN id = :node_id THEN 1 ELSE removed END,
        employee = CASE WHEN employee = :node_id THEN NULL ELSE employee END
    WHERE id = :node_id OR employee = :node_id;
    )");
}

QString SqliteStakeholder::QSNodeTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT lhs_node, id FROM stakeholder_transaction
    WHERE lhs_node = :node_id AND removed = 0
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
    CString& string { QSWriteNodeTrans() };

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

bool SqliteStakeholder::UpdateDateTimePrice(CString& date_time, double unit_price, int trans_id)
{
    QSqlQuery query(*db_);

    auto part = QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        date_time = :date_time, unit_price = :unit_price
    WHERE id = :trans_id
    )");

    query.prepare(part);
    query.bindValue(":trans_id", trans_id);
    query.bindValue(":date_time", date_time);
    query.bindValue(":unit_price", unit_price);

    if (!query.exec()) {
        qWarning() << "Failed in UpdateDatePrice" << query.lastError().text();
        return false;
    }

    return true;
}

void SqliteStakeholder::WriteTransBind(Trans* trans, QSqlQuery& query) const
{
    query.bindValue(":date_time", trans->date_time);
    query.bindValue(":code", trans->code);
    query.bindValue(":lhs_node", trans->lhs_node);
    query.bindValue(":unit_price", trans->unit_price);
    query.bindValue(":description", trans->description);
    query.bindValue(":state", trans->state);
    query.bindValue(":document", trans->document.join(SEMICOLON));
    query.bindValue(":inside_product", trans->rhs_node);
    query.bindValue(":outside_product", trans->helper_node);
}

QMultiHash<int, int> SqliteStakeholder::ReplaceNodeFunction(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };
    QMultiHash<int, int> hash {};

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id) {
            hash.emplace(old_node_id, trans->id);
            trans->lhs_node = new_node_id;
        }

        if (trans->helper_node == old_node_id) {
            trans->helper_node = new_node_id;
        }
    }

    return hash;
}

void SqliteStakeholder::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":lhs_node", *trans_shadow->lhs_node);
    query.bindValue(":unit_price", *trans_shadow->unit_price);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":inside_product", *trans_shadow->rhs_node);
    query.bindValue(":outside_product", *trans_shadow->helper_node);
}

void SqliteStakeholder::UpdateProductReferenceSO(int old_node_id, int new_node_id) const
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

QString SqliteStakeholder::QSReadNodeTransRangeFPTS(CString& in_list) const
{
    return QString(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

QString SqliteStakeholder::QSReadHelperTransRangeFPTS(CString& in_list) const
{
    return QString(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE helper_node IN (%1) AND removed = 0
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
    node->is_helper = query.value("is_helper").toBool();
    node->employee = query.value("employee").toInt();
    node->date_time = query.value("deadline").toString();
    node->first = query.value("payment_period").toInt();
    node->second = query.value("tax_rate").toDouble();
}

void SqliteStakeholder::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->helper_node = query.value("outside_product").toInt();
    trans->lhs_node = query.value("lhs_node").toInt();
    trans->rhs_node = query.value("inside_product").toInt();
    trans->unit_price = query.value("unit_price").toDouble();
    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->state = query.value("state").toBool();
    trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
}
