#include "sqliteorder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"

SqliteOrder::SqliteOrder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteOrder::BuildTreeQueryString() const
{
    return QString(R"(
            SELECT name, id, code, description, note, node_rule, branch, unit, party, employee, date_time, first, second, discount, locked, initial_total, final_total
            FROM %1
            WHERE removed = 0
            )")
        .arg(info_.node);
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
            WriteRelationship(node->id, parent_id, query);
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

QString SqliteOrder::BuildTransShadowListQueryString() const
{
    return QString(R"(
            SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
            FROM %1
            WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
            )")
        .arg(info_.transaction);
}

bool SqliteOrder::InsertTransShadow(TransShadow* trans_shadow)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    INSERT INTO %1 (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":lhs_node", *trans_shadow->node);
    query.bindValue(":lhs_ratio", *trans_shadow->ratio);
    query.bindValue(":lhs_debit", *trans_shadow->debit);
    query.bindValue(":lhs_credit", *trans_shadow->credit);
    query.bindValue(":rhs_node", *trans_shadow->related_node);
    query.bindValue(":rhs_ratio", *trans_shadow->related_ratio);
    query.bindValue(":rhs_debit", *trans_shadow->related_debit);
    query.bindValue(":rhs_credit", *trans_shadow->related_credit);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));

    if (!query.exec()) {
        qWarning() << "Failed to insert record in transaction table" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

void SqliteOrder::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
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
}
