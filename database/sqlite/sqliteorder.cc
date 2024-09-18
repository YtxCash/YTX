#include "sqliteorder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteOrder::SqliteOrder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

bool SqliteOrder::BuildTree(NodeHash& node_hash)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total
    FROM %1
    WHERE removed = 0
)")
                    .arg(info_.node);

    // WHERE unit = 2 AND removed = 0

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Error in order create tree 1 setp " << query.lastError().text();
        return false;
    }

    BuildNodeHash(query, node_hash);
    query.clear();
    ReadRelationship(query, node_hash);

    return true;
}

bool SqliteOrder::InsertNode(int parent_id, Node* node)
{
    if (!node || node->id == -1)
        return false;

    QSqlQuery query(*db_);

    auto part = QString(R"(
    INSERT INTO %1 (name, code, description, note, node_rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit, :party, :employee, :date_time, :first, :second, :discount, :locked, :initial_total, :final_total)
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
            query.bindValue(":party", node->party);
            query.bindValue(":employee", node->employee);
            query.bindValue(":date_time", node->date_time);
            query.bindValue(":first", node->first);
            query.bindValue(":second", node->second);
            query.bindValue(":discount", node->discount);
            query.bindValue(":locked", node->locked);
            query.bindValue(":initial_total", node->initial_total);
            query.bindValue(":final_total", node->final_total);

            if (!query.exec()) {
                qWarning() << "Failed to insert order_node record: " << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            WriteRelationship(query, node->id, parent_id);
            return true;
        })) {
        qWarning() << "Failed to insert order record";
        return false;
    }

    return true;
}

void SqliteOrder::NodeLeafTotal(Node* node)
{
    if (!node || node->id == -1 || node->branch)
        return;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    // todo!
}

bool SqliteOrder::UpdateNodeSimple(const Node* node)
{
    if (!node || !db_) {
        qWarning() << "Invalid node or database pointer";
        return false;
    }

    QSqlQuery query(*db_);

    auto part = QString(R"(
    UPDATE %1 SET
    code = :code, description = :description, note = :note, date_time = :date_time,
    node_rule = :node_rule, first = :first, employee = :employee, second = :second
    WHERE id = :id
)")
                    .arg(info_.node);

    query.prepare(part);
    query.bindValue(":id", node->id);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":date_time", node->date_time);
    query.bindValue(":node_rule", node->node_rule);
    query.bindValue(":first", node->first);
    query.bindValue(":employee", node->employee);
    query.bindValue(":second", node->second);

    if (!query.exec()) {
        qWarning() << "Failed to update node simple (ID:" << node->id << "):" << query.lastError().text();
        return false;
    }

    return true;
}

void SqliteOrder::BuildTransList(TransList& trans_list, int node_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM %1
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Error in Construct Table" << query.lastError().text();
        return;
    }

    QueryTransList(trans_list, node_id, query);
}

bool SqliteOrder::InsertTrans(Trans* trans)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    INSERT INTO %1 (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":date_time", *trans->date_time);
    query.bindValue(":lhs_node", *trans->node);
    query.bindValue(":lhs_ratio", *trans->ratio);
    query.bindValue(":lhs_debit", *trans->debit);
    query.bindValue(":lhs_credit", *trans->credit);
    query.bindValue(":rhs_node", *trans->related_node);
    query.bindValue(":rhs_ratio", *trans->related_ratio);
    query.bindValue(":rhs_debit", *trans->related_debit);
    query.bindValue(":rhs_credit", *trans->related_credit);
    query.bindValue(":state", *trans->state);
    query.bindValue(":description", *trans->description);
    query.bindValue(":code", *trans->code);
    query.bindValue(":document", trans->document->join(SEMICOLON));

    if (!query.exec()) {
        qWarning() << "Failed to insert record in transaction table" << query.lastError().text();
        return false;
    }

    *trans->id = query.lastInsertId().toInt();
    transaction_hash_.insert(*trans->id, last_transaction_);
    return true;
}

void SqliteOrder::BuildTransList(TransList& trans_list, int node_id, const QList<int>& trans_id_list)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QStringList list {};

    for (const auto& id : trans_id_list)
        list.append(QString::number(id));

    auto part = QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM %1
    WHERE id IN (%2) AND (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
)")
                    .arg(info_.transaction, list.join(", "));

    query.prepare(part);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Error in ConstructTable 1st" << query.lastError().text();
        return;
    }

    QueryTransList(trans_list, node_id, query);
}

void SqliteOrder::BuildNodeHash(QSqlQuery& query, NodeHash& node_hash)
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
        node->party = query.value("party").toInt();
        node->employee = query.value("employee").toInt();
        node->date_time = query.value("date_time").toString();
        node->first = query.value("first").toInt();
        node->second = query.value("second").toDouble();
        node->discount = query.value("discount").toDouble();
        node->locked = query.value("locked").toBool();
        node->initial_total = query.value("initial_total").toDouble();
        node->final_total = query.value("final_total").toDouble();
        node_hash.insert(node_id, node);
    }
}

void SqliteOrder::QueryTransList(TransList& trans_list, int node_id, QSqlQuery& query)
{
    Trans* trans {};
    Transaction* transaction {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();
        trans = ResourcePool<Trans>::Instance().Allocate();

        if (transaction_hash_.contains(id)) {
            transaction = transaction_hash_.value(id);
            Convert(transaction, trans, node_id == transaction->lhs_node);
            trans_list.emplaceBack(trans);
            continue;
        }

        transaction = ResourcePool<Transaction>::Instance().Allocate();
        transaction->id = id;

        transaction->lhs_node = query.value("lhs_node").toInt();
        transaction->lhs_ratio = query.value("lhs_ratio").toDouble();
        transaction->lhs_debit = query.value("lhs_debit").toDouble();
        transaction->lhs_credit = query.value("lhs_credit").toDouble();

        transaction->rhs_node = query.value("rhs_node").toInt();
        transaction->rhs_ratio = query.value("rhs_ratio").toDouble();
        transaction->rhs_debit = query.value("rhs_debit").toDouble();
        transaction->rhs_credit = query.value("rhs_credit").toDouble();

        transaction->code = query.value("code").toString();
        transaction->description = query.value("description").toString();
        transaction->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
        transaction->date_time = query.value("date_time").toString();
        transaction->state = query.value("state").toBool();

        transaction_hash_.insert(id, transaction);
        Convert(transaction, trans, node_id == transaction->lhs_node);
        trans_list.emplaceBack(trans);
    }
}
