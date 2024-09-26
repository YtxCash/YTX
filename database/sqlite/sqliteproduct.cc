#include "sqliteproduct.h"

#include <QSqlError>
#include <QSqlQuery>

SqliteProduct::SqliteProduct(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteProduct::BuildTreeQueryString() const
{
    return QString(R"(
            SELECT name, id, code, description, note, node_rule, branch, unit, commission, unit_price, initial_total, final_total
            FROM %1
            WHERE removed = 0
            )")
        .arg(info_.node);
}

bool SqliteProduct::InsertNode(int parent_id, Node* node)
{
    if (!node || node->id == -1)
        return false;

    QSqlQuery query(*db_);

    auto part = QString(R"(
    INSERT INTO %1 (name, code, description, note, node_rule, branch, unit, commission, unit_price)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit, :commission, :unit_price)
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
            query.bindValue(":commission", node->second);
            query.bindValue(":unit_price", node->discount);

            if (!query.exec()) {
                qWarning() << "Failed to insert node record: " << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            WriteRelationship(node->id, parent_id, query);
            return true;
        })) {
        qWarning() << "Failed to insert record";
        return false;
    }

    return true;
}

bool SqliteProduct::ExternalReference(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string = R"(
    SELECT COUNT(*)
    FROM (
        SELECT 1 FROM stakeholder_transaction WHERE rhs_node = :node_id AND removed = 0
        UNION ALL
        SELECT 1 FROM sales_transaction WHERE lhs_node = :node_id AND removed = 0
        UNION ALL
        SELECT 1 FROM purchase_transaction WHERE lhs_node = :node_id AND removed = 0
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

void SqliteProduct::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->node_rule = query.value("node_rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->second = query.value("commission").toDouble();
    node->discount = query.value("unit_price").toDouble();
    node->initial_total = query.value("initial_total").toDouble();
    node->final_total = query.value("final_total").toDouble();
}
