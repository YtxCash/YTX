#include "sqliteproduct.h"

#include <QSqlError>
#include <QSqlQuery>

#include "global/resourcepool.h"

SqliteProduct::SqliteProduct(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

bool SqliteProduct::BuildTree(NodeHash& node_hash)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, commission, unit_price, initial_total, final_total
    FROM %1
    WHERE removed = 0
)")
                    .arg(info_.node);

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Error in product create tree 1 setp " << query.lastError().text();
        return false;
    }

    BuildNodeHash(query, node_hash);
    query.clear();
    ReadRelationship(query, node_hash);

    return true;
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
            query.bindValue(":unit_price", node->third);

            if (!query.exec()) {
                qWarning() << "Failed to insert node record: " << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            WriteRelationship(query, node->id, parent_id);
            return true;
        })) {
        qWarning() << "Failed to insert record";
        return false;
    }

    return true;
}

bool SqliteProduct::NodeExternalReferences(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string = R"(
    SELECT COUNT(*)
    FROM (
        SELECT 1 FROM stakeholder_transaction WHERE rhs_node = :node_id AND removed = 0
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

void SqliteProduct::BuildNodeHash(QSqlQuery& query, NodeHash& node_hash)
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
        node->second = query.value("commission").toDouble();
        node->third = query.value("unit_price").toDouble();
        node->initial_total = query.value("initial_total").toDouble();
        node->final_total = query.value("final_total").toDouble();

        node_hash.insert(node_id, node);
    }
}
