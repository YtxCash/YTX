#include "sqlitestakeholder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteStakeholder::SqliteStakeholder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

bool SqliteStakeholder::BuildTree(NodeHash& node_hash)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, employee, deadline, payment_period, tax_rate
    FROM %1
    WHERE removed = 0
)")
                    .arg(info_.node);

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::Tree(NodeHash& node_hash) 1st" << query.lastError().text();
        return false;
    }

    BuildNodeHash(query, node_hash);
    query.clear();
    ReadRelationship(query, node_hash);

    return true;
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
            WriteRelationship(query, node->id, parent_id);
            return true;
        })) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::Insert(int parent_id, Node* node) end";
        return false;
    }

    return true;
}

bool SqliteStakeholder::NodeInternalReferences(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string = QString(R"(
    SELECT COUNT(*) FROM %1
    WHERE outside = :node_id AND removed = 0
)")
                      .arg(info_.transaction);

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::InternalReferences(int node_id) const" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool SqliteStakeholder::NodeExternalReferences(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string = R"(
    SELECT COUNT(*)
    FROM (
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

bool SqliteStakeholder::UpdateNodeSimple(const Node* node)
{
    if (!node || !db_) {
        qWarning() << "Invalid node or database pointer";
        return false;
    }

    QSqlQuery query(*db_);

    auto part = QString(R"(
    UPDATE %1 SET
    code = :code, description = :description, note = :note, deadline = :deadline, unit = :unit,
    node_rule = :node_rule, payment_period = :payment_period, employee = :employee, tax_rate = :tax_rate
    WHERE id = :id
)")
                    .arg(info_.node);

    query.prepare(part);
    query.bindValue(":id", node->id);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":node_rule", node->node_rule);
    query.bindValue(":unit", node->unit);
    query.bindValue(":employee", node->employee);
    query.bindValue(":deadline", node->date_time);
    query.bindValue(":payment_period", node->first);
    query.bindValue(":tax_rate", node->second);

    if (!query.exec()) {
        qWarning() << "Failed to update node simple (ID:" << node->id << "):" << query.lastError().text();
        return false;
    }

    return true;
}

bool SqliteStakeholder::RemoveNode(int node_id, bool branch)
{
    QSqlQuery query(*db_);

    auto part_1st = QString(R"(
    UPDATE %1
    SET removed = 1
    WHERE id = :node_id
)")
                        .arg(info_.node);

    auto part_2nd = QString(R"(
    UPDATE %1
    SET removed = 1
    WHERE outside = :node_id
)")
                        .arg(info_.transaction);

    if (branch) {
        part_2nd = QString(R"(
        WITH related_nodes AS (
            SELECT DISTINCT fp1.ancestor, fp2.descendant
            FROM %1 AS fp1
            INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
            WHERE fp2.ancestor = :node_id AND fp2.descendant != :node_id
            AND fp1.ancestor != :node_id
        )
        UPDATE %1
        SET distance = distance - 1
        WHERE ancestor IN (SELECT ancestor FROM related_nodes)
        AND descendant IN (SELECT descendant FROM related_nodes)
    )")
                       .arg(info_.path);
    }

    auto part_3rd = QString("DELETE FROM %1 WHERE (descendant = :node_id OR ancestor = :node_id) AND distance !=0").arg(info_.path);

    if (!DBTransaction([&]() {
            query.prepare(part_1st);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node record 1st step: " << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(part_2nd);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node_path record 2nd step: " << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(part_3rd);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node_path record 3rd step: " << query.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed to remove node";
        return false;
    }

    return true;
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

void SqliteStakeholder::BuildTransShadowList(TransShadowList& trans_shadow_list, int outside_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT id, date_time, code, outside, unit_price, commission, description, document, state, inside
    FROM %1
    WHERE outside = :outside_id AND removed = 0
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":outside_id", outside_id);

    if (!query.exec()) {
        qWarning() << "sqlstakeholder: SPTransList SqlStakeholder::TransList(int outside_id)" << query.lastError().text();
        return;
    }

    QueryTransShadowList(trans_shadow_list, outside_id, query);
}

bool SqliteStakeholder::InsertTransShadow(TransShadow* trans_shadow)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    INSERT INTO %1
    (date_time, code, outside, unit_price, commission, description, document, state, inside)
    VALUES
    (:date_time, :code, :outside, :unit_price, :commission, :description, :document, :state, :inside)
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":outside", *trans_shadow->node);
    query.bindValue(":unit_price", *trans_shadow->ratio);
    query.bindValue(":commission", *trans_shadow->related_debit);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->related_ratio);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":inside", *trans_shadow->related_node);

    if (!query.exec()) {
        qWarning() << "sqlstakeholder: bool SqlStakeholder::Insert(Trans*trans_shadow)" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

void SqliteStakeholder::BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QStringList list {};

    for (const auto& id : trans_id_list)
        list.append(QString::number(id));

    auto part = QString(R"(
    SELECT id, date_time, code, outside, unit_price, commission, description, document, state, inside
    FROM %1
    WHERE id IN (%2) AND outside = :node_id AND removed = 0
)")
                    .arg(info_.transaction, list.join(", "));

    query.prepare(part);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "sqlstakeholder: SPTransList SqlStakeholder::TransList(int node_id, const QList<int>& trans_id_list)" << query.lastError().text();
        return;
    }

    QueryTransShadowList(trans_shadow_list, node_id, query);
}

void SqliteStakeholder::BuildNodeHash(QSqlQuery& query, NodeHash& node_hash)
{
    int node_id {};
    Node* node {};

    while (query.next()) {
        node = ResourcePool<Node>::Instance().Allocate();
        node_id = query.value("id").toInt();

        node->id = node_id;
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

        node_hash.insert(node_id, node);
    }
}

void SqliteStakeholder::QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query)
{
    Q_UNUSED(node_id)

    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        if (trans_hash_.contains(id)) {
            trans = trans_hash_.value(id);
            Convert(trans, trans_shadow, true);
            trans_shadow_list.emplaceBack(trans_shadow);
            continue;
        }

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans->id = id;

        trans->lhs_node = query.value("outside").toInt();
        trans->rhs_node = query.value("inside").toInt();
        trans->lhs_ratio = query.value("unit_price").toDouble();
        trans->rhs_ratio = query.value("commission").toDouble();
        trans->code = query.value("code").toString();
        trans->description = query.value("description").toString();
        trans->state = query.value("state").toBool();
        trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
        trans->date_time = query.value("date_time").toString();

        trans_hash_.insert(id, trans);
        Convert(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
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
