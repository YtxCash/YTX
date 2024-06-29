#include "sqlproduct.h"

#include <QSqlError>
#include <QSqlQuery>

#include "global/nodepool.h"

SqlProduct::SqlProduct(const Info* info, QObject* parent)
    : Sql(info, parent)
{
}

bool SqlProduct::Tree(NodeHash& node_hash)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT name, id, code, unit_price, commission, description, note, node_rule, branch, unit, quantity_total, amount_total
    FROM %1
    WHERE removed = 0
)")
                    .arg(info_->node);

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Error in product create tree 1 setp " << query.lastError().text();
        return false;
    }

    CreateNodeHash(query, node_hash);
    query.clear();
    ReadRelationship(query, node_hash);

    return true;
}

bool SqlProduct::Insert(int parent_id, Node* node)
{
    if (!node || node->id == -1)
        return false;

    QSqlQuery query(*db_);

    auto part = QString(R"(
    INSERT INTO %1 (name, code, unit_price, commission, description, note, node_rule, branch, unit)
    VALUES (:name, :code, :unit_price, :commission, :description, :note, :node_rule, :branch, :unit)
)")
                    .arg(info_->node);

    if (!DBTransaction([&]() {
            // 插入节点记录
            query.prepare(part);
            query.bindValue(":name", node->name);
            query.bindValue(":code", node->code);
            query.bindValue(":unit_price", node->third_property);
            query.bindValue(":commission", node->fourth_property);
            query.bindValue(":description", node->description);
            query.bindValue(":note", node->note);
            query.bindValue(":node_rule", node->node_rule);
            query.bindValue(":branch", node->branch);
            query.bindValue(":unit", node->unit);

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

void SqlProduct::LeafTotal(Node* node)
{
    if (!node || node->id == -1 || node->branch)
        return;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT lhs_debit AS debit, lhs_credit AS credit, lhs_ratio AS ratio FROM %1
    WHERE lhs_node = (:node_id) AND removed = 0
    UNION ALL
    SELECT rhs_debit, rhs_credit, rhs_ratio FROM %1
    WHERE rhs_node = (:node_id) AND removed = 0
)")
                    .arg(info_->transaction);

    query.prepare(part);
    query.bindValue(":node_id", node->id);
    if (!query.exec())
        qWarning() << "Error in calculate node total setp " << query.lastError().text();

    double quantity_total_debit { 0.0 };
    double quantity_total_credit { 0.0 };
    double amount_total_debit { 0.0 };
    double amount_total_credit { 0.0 };
    bool node_rule { node->node_rule };

    double unit_cost { 0.0 };
    double debit { 0.0 };
    double credit { 0.0 };

    while (query.next()) {
        unit_cost = query.value("ratio").toDouble();

        debit = query.value("debit").toDouble();
        credit = query.value("credit").toDouble();

        amount_total_debit += debit * unit_cost;
        amount_total_credit += credit * unit_cost;

        quantity_total_debit += debit;
        quantity_total_credit += credit;
    }

    node->initial_total = node_rule ? (quantity_total_credit - quantity_total_debit) : (quantity_total_debit - quantity_total_credit);
    node->final_total = node_rule ? (amount_total_credit - amount_total_debit) : (amount_total_debit - amount_total_credit);
}

bool SqlProduct::ExternalReferences(int node_id) const
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

void SqlProduct::CreateNodeHash(QSqlQuery& query, NodeHash& node_hash)
{
    int node_id {};
    Node* node {};

    while (query.next()) {
        node = NodePool::Instance().Allocate();
        node_id = query.value("id").toInt();

        node->id = node_id;
        node->name = query.value("name").toString();
        node->code = query.value("code").toString();
        node->third_property = query.value("unit_price").toDouble();
        node->fourth_property = query.value("commission").toDouble();
        node->description = query.value("description").toString();
        node->note = query.value("note").toString();
        node->node_rule = query.value("node_rule").toBool();
        node->branch = query.value("branch").toBool();
        node->unit = query.value("unit").toInt();
        node->initial_total = query.value("quantity_total").toDouble();
        node->final_total = query.value("amount_total").toDouble();

        node_hash.insert(node_id, node);
    }
}
