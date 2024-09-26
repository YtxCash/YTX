#include "sqlitestakeholder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteStakeholder::SqliteStakeholder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteStakeholder::BuildTreeQueryString() const
{
    return QString(R"(
            SELECT name, id, code, description, note, node_rule, branch, unit, employee, deadline, payment_period, tax_rate
            FROM %1
            WHERE removed = 0
            )")
        .arg(info_.node);
}

bool SqliteStakeholder::InsertNode(int parent_id, Node* node)
{
    // root_'s id is -1
    if (!node || node->id == -1)
        return false;

    QSqlQuery query(*db_);

    auto part = QString(R"(
    INSERT INTO %1 (name, code, description, note, node_rule, branch, unit, employee, deadline, payment_period, tax_rate)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit, :employee, :deadline, :payment_period, :tax_rate)
)")
                    .arg(info_.node);

    if (!DBTransaction([&]() {
            // 插入节点记录
            query.prepare(part);
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

            if (!query.exec()) {
                qWarning() << "sqlstakeholder: bool SqlStakeholder::Insert(int parent_id, Node* node) 1st" << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            WriteRelationship(node->id, parent_id, query);
            return true;
        })) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::Insert(int parent_id, Node* node) end";
        return false;
    }

    return true;
}

bool SqliteStakeholder::ExternalReference(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string = R"(
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
)";

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Failed to count times " << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

QString SqliteStakeholder::RemoveNodeQueryStringSecond() const
{
    return QString(R"(
            UPDATE %1
            SET removed = 1
            WHERE lhs_node = :node_id
            )")
        .arg(info_.transaction);
}

QString SqliteStakeholder::InternalReferenceQueryString() const
{
    return QString(R"(
            SELECT COUNT(*) FROM %1
            WHERE lhs_node = :node_id AND removed = 0
            )")
        .arg(info_.transaction);
}

bool SqliteStakeholder::RRemoveMulti(int node_id)
{
    auto list { TransID(node_id) };

    // begin deal with database
    QSqlQuery query(*db_);

    auto part = QString(R"(
    UPDATE %1
    SET removed = 1
    WHERE lhs_node = :node_id
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":node_id", node_id);
    if (!query.exec()) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::RRemoveMulti(int node_id)" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SFreeView(node_id);
    emit SRemoveNode(node_id);

    // begin deal with trans hash
    for (int trans_id : list)
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    // end deal with trans hash

    return true;
}

bool SqliteStakeholder::RReplaceMulti(int old_node_id, int new_node_id)
{
    auto node_trans { RelatedNodeTrans(old_node_id) };

    // begin deal with trans hash
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto& trans : const_trans_hash)
        if (trans->lhs_node == old_node_id)
            trans->lhs_node = new_node_id;
    // end deal with trans hash

    // begin deal with database
    QSqlQuery query(*db_);
    auto part = QString(R"(
    UPDATE %1
    SET lhs_node = :new_node_id
    WHERE lhs_node = :old_node_id
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::RReplaceMulti(int old_node_id, int new_node_id)" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SFreeView(old_node_id);
    emit SRemoveNode(old_node_id);
    emit SMoveMulti(info_.section, old_node_id, new_node_id, node_trans.values());

    return true;
}

bool SqliteStakeholder::RReplaceReferences(Section origin, int old_node_id, int new_node_id)
{
    Q_UNUSED(origin)

    QSqlQuery query(*db_);
    auto part = QString(R"(
    UPDATE %1
    SET rhs_node = :new_node_id
    WHERE rhs_node = :old_node_id
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":old_node_id", old_node_id);
    query.bindValue(":new_node_id", new_node_id);
    if (!query.exec()) {
        qWarning() << "sqlstakeholder: " << query.lastError().text();
        return false;
    }

    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto& trans : const_trans_hash)
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;

    return true;
}

QString SqliteStakeholder::BuildTransShadowListQueryString() const
{
    return QString(R"(
            SELECT id, date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node
            FROM %1
            WHERE lhs_node = :lhs_node_id AND removed = 0
            )")
        .arg(info_.transaction);
}

bool SqliteStakeholder::InsertTransShadow(TransShadow* trans_shadow)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    INSERT INTO %1
    (date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node)
    VALUES
    (:date_time, :code, :lhs_node, :lhs_ratio, :description, :document, :state, :rhs_node)
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":lhs_node", *trans_shadow->node);
    query.bindValue(":lhs_ratio", *trans_shadow->ratio);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":rhs_node", *trans_shadow->related_node);

    if (!query.exec()) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::Insert(Trans*trans_shadow)" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

QString SqliteStakeholder::BuildTransShadowListRangQueryString(QStringList& list) const
{
    return QString(R"(
            SELECT id, date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node
            FROM %1
            WHERE id IN (%2) AND lhs_node = :node_id AND removed = 0
            )")
        .arg(info_.transaction, list.join(", "));
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

QList<int> SqliteStakeholder::TransID(int node_id, bool lhs)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QList<int> list {};
    auto part = QString(R"(
    SELECT id FROM %1
    WHERE lhs_node = :node_id AND removed = 0
)")
                    .arg(info_.transaction);

    if (!lhs)
        part = QString(R"(
        SELECT id FROM %1
        WHERE rhs_node = :node_id AND removed = 0
)")
                   .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "sqlstakeholder: QList<int> SqlStakeholder::TransID(int node_id, bool lhs)" << query.lastError().text();
        return QList<int>();
    }

    while (query.next())
        list.emplaceBack(query.value("id").toInt());

    return list;
}
