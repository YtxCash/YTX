#include "sqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "global/sqlconnection.h"

Sqlite::Sqlite(CInfo& info, QObject* parent)
    : QObject(parent)
    , db_ { SqlConnection::Instance().Allocate(info.section) }
    , info_ { info }
{
}

Sqlite::~Sqlite() { qDeleteAll(transaction_hash_); }

bool Sqlite::RRemoveMulti(int node_id)
{
    auto node_trans { RelatedNodeTrans(node_id) };
    auto trans_id_list { node_trans.values() };

    // begin deal with database
    QSqlQuery query(*db_);

    QStringList list {};
    for (const int& id : trans_id_list)
        list.append(QString::number(id));

    auto part = QString(R"(
    UPDATE %1
    SET removed = 1
    WHERE id IN (%2)
)")
                    .arg(info_.transaction, list.join(", "));

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Failed to remove record in transaction table" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SFreeView(node_id);
    emit SRemoveNode(node_id);
    emit SRemoveMulti(node_trans);
    emit SUpdateMultiTotal(node_trans.uniqueKeys());

    // begin deal with transaction hash
    for (int trans_id : trans_id_list)
        ResourcePool<Transaction>::Instance().Recycle(transaction_hash_.take(trans_id));
    // end deal with transaction hash

    return true;
}

bool Sqlite::RReplaceMulti(int old_node_id, int new_node_id)
{
    auto node_trans { RelatedNodeTrans(old_node_id) };
    bool free { !node_trans.contains(new_node_id) };

    node_trans.remove(new_node_id);

    // begin deal with transaction hash
    const auto& const_transaction_hash { std::as_const(transaction_hash_) };

    for (auto& transaction : const_transaction_hash) {
        if (transaction->lhs_node == old_node_id && transaction->rhs_node != new_node_id)
            transaction->lhs_node = new_node_id;

        if (transaction->rhs_node == old_node_id && transaction->lhs_node != new_node_id)
            transaction->rhs_node = new_node_id;
    }
    // end deal with transaction hash

    // begin deal with database
    QSqlQuery query(*db_);
    auto part = QString(R"(
    UPDATE %1 SET
    lhs_node = CASE
        WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id
        ELSE lhs_node
    END,
    rhs_node = CASE
        WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id
        ELSE rhs_node
    END
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Error in replace node setp" << query.lastError().text();
        return false;
    }
    // end deal with database

    if (free) {
        emit SFreeView(old_node_id);
        emit SRemoveNode(old_node_id);
    }

    emit SMoveMulti(info_.section, old_node_id, new_node_id, node_trans.values());
    emit SUpdateMultiTotal(QList { old_node_id, new_node_id });
    emit SReplaceReferences(info_.section, old_node_id, new_node_id);

    return true;
}

bool Sqlite::BuildTree(NodeHash& node_hash)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, initial_total, final_total
    FROM %1
    WHERE removed = 0
)")
                    .arg(info_.node);

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Error in finance create tree 1 setp " << query.lastError().text();
        return false;
    }

    BuildNodeHash(query, node_hash);
    query.clear();
    ReadRelationship(query, node_hash);
    return true;
}

bool Sqlite::InsertNode(int parent_id, Node* node)
{ // root_'s id is -1
    if (!node || node->id == -1)
        return false;

    QSqlQuery query(*db_);

    auto part = QString(R"(
    INSERT INTO %1 (name, code, description, note, node_rule, branch, unit)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit)
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

            if (!query.exec()) {
                qWarning() << "Failed to insert finance node record: " << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            WriteRelationship(query, node->id, parent_id);
            return true;
        })) {
        qWarning() << "Failed to insert finance record";
        return false;
    }

    return true;
}

void Sqlite::NodeLeafTotal(Node* node)
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
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":node_id", node->id);
    if (!query.exec())
        qWarning() << "Error in calculate finance total 1st setp " << query.lastError().text();

    double initial_total_debit { 0.0 };
    double initial_total_credit { 0.0 };
    double final_total_debit { 0.0 };
    double final_total_credit { 0.0 };
    bool node_rule { node->node_rule };

    double ratio { 0.0 };
    double debit { 0.0 };
    double credit { 0.0 };

    while (query.next()) {
        ratio = query.value("ratio").toDouble();

        debit = query.value("debit").toDouble();
        credit = query.value("credit").toDouble();

        final_total_debit += debit * ratio;
        final_total_credit += credit * ratio;

        initial_total_debit += debit;
        initial_total_credit += credit;
    }

    int sign = node_rule ? 1 : -1;
    node->initial_total = sign * (initial_total_credit - initial_total_debit);
    node->final_total = sign * (final_total_credit - final_total_debit);
}

bool Sqlite::UpdateNodeSimple(const Node* node)
{
    if (!node || !db_) {
        qWarning() << "Invalid node or database pointer";
        return false;
    }

    QSqlQuery query(*db_);

    auto part = QString(R"(
    UPDATE %1 SET
    code = :code, description = :description, note = :note
    WHERE id = :id
)")
                    .arg(info_.node);

    query.prepare(part);
    query.bindValue(":id", node->id);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);

    if (!query.exec()) {
        qWarning() << "Failed to update node simple (ID:" << node->id << "):" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::RemoveNode(int node_id, bool branch)
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
    WHERE lhs_node = :node_id OR rhs_node = :node_id
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

    //     auto part_22nd = QString("UPDATE %1 "
    //                                   "SET distance = distance - 1 "
    //                                   "WHERE (descendant IN (SELECT descendant
    //                                   FROM %1 " "WHERE ancestor = :node_id AND
    //                                   ancestor != descendant) AND ancestor IN
    //                                   (SELECT ancestor FROM %1 " "WHERE
    //                                   descendant = :node_id AND ancestor !=
    //                                   descendant)) ")
    //                               .arg(info_.path);

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

bool Sqlite::DragNode(int destination_node_id, int node_id)
{
    QSqlQuery query(*db_);

    //    auto part_1st = QString("DELETE FROM %1 WHERE (descendant IN
    //    (SELECT descendant FROM "
    //                                  "%1 WHERE ancestor = :node_id) AND
    //                                  ancestor IN (SELECT ancestor FROM "
    //                                  "%1 WHERE descendant = :node_id AND
    //                                  ancestor != descendant)) ")
    //    .arg(path_);

    auto part_1st = QString(R"(
    WITH related_nodes AS (
        SELECT DISTINCT fp1.ancestor, fp2.descendant
        FROM %1 AS fp1
        INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
        WHERE fp2.ancestor = :node_id AND fp1.ancestor != :node_id
    )
    DELETE FROM %1
    WHERE ancestor IN (SELECT ancestor FROM related_nodes)
    AND descendant IN (SELECT descendant FROM related_nodes)
)")
                        .arg(info_.path);

    auto part_2nd = QString(R"(
    INSERT INTO %1 (ancestor, descendant, distance)
    SELECT fp1.ancestor, fp2.descendant, fp1.distance + fp2.distance + 1
    FROM %1 AS fp1
    INNER JOIN %1 AS fp2
    WHERE fp1.descendant = :destination_node_id AND fp2.ancestor = :node_id
)")
                        .arg(info_.path);

    if (!DBTransaction([&]() {
            // 第一个查询
            query.prepare(part_1st);
            query.bindValue(":node_id", node_id);

            if (!query.exec()) {
                qWarning() << "Failed to drag node_path record 1st step: " << query.lastError().text();
                return false;
            }

            query.clear();

            // 第二个查询
            query.prepare(part_2nd);
            query.bindValue(":node_id", node_id);
            query.bindValue(":destination_node_id", destination_node_id);

            if (!query.exec()) {
                qWarning() << "Failed to drag node_path record 2nd step: " << query.lastError().text();
                return false;
            }
            return true;
        })) {
        qWarning() << "Failed to drag node";
        return false;
    }

    return true;
}

bool Sqlite::NodeInternalReferences(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string = QString(R"(
    SELECT COUNT(*) FROM %1
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
)")
                      .arg(info_.transaction);

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Failed to count times " << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

void Sqlite::BuildTransList(TransList& trans_list, int node_id)
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

void Sqlite::Convert(Transaction* transaction, Trans* trans, bool left)
{
    trans->id = &transaction->id;
    trans->state = &transaction->state;
    trans->date_time = &transaction->date_time;
    trans->code = &transaction->code;
    trans->document = &transaction->document;
    trans->description = &transaction->description;

    if (left) {
        trans->node = &transaction->lhs_node;
        trans->ratio = &transaction->lhs_ratio;
        trans->debit = &transaction->lhs_debit;
        trans->credit = &transaction->lhs_credit;

        trans->related_node = &transaction->rhs_node;
        trans->related_ratio = &transaction->rhs_ratio;
        trans->related_debit = &transaction->rhs_debit;
        trans->related_credit = &transaction->rhs_credit;

        return;
    }

    trans->node = &transaction->rhs_node;
    trans->ratio = &transaction->rhs_ratio;
    trans->debit = &transaction->rhs_debit;
    trans->credit = &transaction->rhs_credit;

    trans->related_node = &transaction->lhs_node;
    trans->related_ratio = &transaction->lhs_ratio;
    trans->related_debit = &transaction->lhs_debit;
    trans->related_credit = &transaction->lhs_credit;
}

bool Sqlite::InsertTrans(Trans* trans)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    INSERT INTO %1
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
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

bool Sqlite::RemoveTransaction(int trans_id)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    UPDATE %1
    SET removed = 1
    WHERE id = :trans_id
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":trans_id", trans_id);
    if (!query.exec()) {
        qWarning() << "Failed to remove record in transaction table" << query.lastError().text();
        return false;
    }

    ResourcePool<Transaction>::Instance().Recycle(transaction_hash_.take(trans_id));
    return true;
}

bool Sqlite::UpdateTransaction(int trans_id)
{
    QSqlQuery query(*db_);
    auto transaction { transaction_hash_.value(trans_id) };

    auto part = QString(R"(
    UPDATE %1 SET
    lhs_node = :lhs_node, lhs_ratio = :lhs_ratio, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
    rhs_node = :rhs_node, rhs_ratio = :rhs_ratio, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :id
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":lhs_node", transaction->lhs_node);
    query.bindValue(":lhs_ratio", transaction->lhs_ratio);
    query.bindValue(":lhs_debit", transaction->lhs_debit);
    query.bindValue(":lhs_credit", transaction->lhs_credit);
    query.bindValue(":rhs_node", transaction->rhs_node);
    query.bindValue(":rhs_ratio", transaction->rhs_ratio);
    query.bindValue(":rhs_debit", transaction->rhs_debit);
    query.bindValue(":rhs_credit", transaction->rhs_credit);
    query.bindValue(":id", trans_id);

    if (!query.exec()) {
        qWarning() << "Failed to update record in transaction table 1st " << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::UpdateField(CString& table, CVariant& new_value, CString& field, int id)
{
    QSqlQuery query(*db_);

    auto part = QString(R"(
    UPDATE %1
    SET %2 = :value
    WHERE id = :id
)")
                    .arg(table, field);

    query.prepare(part);
    query.bindValue(":id", id);
    query.bindValue(":value", new_value);

    if (!query.exec()) {
        qWarning() << "Failed to update record: " << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::UpdateCheckState(CString& column, CVariant& value, Check state)
{
    QSqlQuery query(*db_);

    auto part = QString(R"(
    UPDATE %1
    SET %2 = :value
)")
                    .arg(info_.transaction, column);

    if (state == Check::kReverse)
        part = QString("UPDATE %1 "
                       "SET %2 = NOT %2 ")
                   .arg(info_.transaction, column);

    query.prepare(part);
    query.bindValue(":value", value);

    if (!query.exec()) {
        qWarning() << "Failed to update record in transaction table " << query.lastError().text();
        return false;
    }

    return true;
}

void Sqlite::BuildTransList(TransList& trans_list, int node_id, const QList<int>& trans_id_list)
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

Trans* Sqlite::AllocateTrans()
{
    last_transaction_ = ResourcePool<Transaction>::Instance().Allocate();
    auto trans { ResourcePool<Trans>::Instance().Allocate() };

    Convert(last_transaction_, trans, true);
    return trans;
}

void Sqlite::BuildNodeHash(QSqlQuery& query, NodeHash& node_hash)
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
        node->initial_total = query.value("initial_total").toDouble();
        node->final_total = query.value("final_total").toDouble();

        node_hash.insert(node_id, node);
    }
}

bool Sqlite::DBTransaction(std::function<bool()> function)
{
    if (db_->transaction() && function() && db_->commit()) {
        return true;
    } else {
        db_->rollback();
        qWarning() << "Transaction failed";
        return false;
    }
}

void Sqlite::ReadRelationship(QSqlQuery& query, const NodeHash& node_hash)
{
    auto part = QString(R"(
    SELECT ancestor, descendant
    FROM %1
    WHERE distance = 1
)")
                    .arg(info_.path);

    query.prepare(part);
    if (!query.exec())
        qWarning() << "Error in sql ReadRelationship " << query.lastError().text();

    int ancestor_id {};
    int descendant_id {};
    Node* ancestor {};
    Node* descendant {};

    while (query.next()) {
        ancestor_id = query.value("ancestor").toInt();
        descendant_id = query.value("descendant").toInt();

        ancestor = node_hash.value(ancestor_id);
        descendant = node_hash.value(descendant_id);

        ancestor->children.emplaceBack(descendant);
        descendant->parent = ancestor;
    }
}

void Sqlite::WriteRelationship(QSqlQuery& query, int node_id, int parent_id)
{
    auto part = QString(R"(
    INSERT INTO %1 (ancestor, descendant, distance)
    SELECT ancestor, :node_id, distance + 1 FROM %1
    WHERE descendant = :parent
    UNION ALL
    SELECT :node_id, :node_id, 0
)")
                    .arg(info_.path);

    query.prepare(part);
    query.bindValue(":node_id", node_id);
    query.bindValue(":parent", parent_id);

    if (!query.exec()) {
        qWarning() << "Error in sql WriteRelationship " << query.lastError().text();
        return;
    }
}

QMultiHash<int, int> Sqlite::RelatedNodeTrans(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QString(R"(
    SELECT lhs_node, id FROM %1
    WHERE rhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT rhs_node, id FROM %1
    WHERE lhs_node = :node_id AND removed = 0
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Error in RelatedNodeAndTrans 1st" << query.lastError().text();
    }

    QMultiHash<int, int> hash {};
    int related_node_id {};
    int trans_id {};

    while (query.next()) {
        related_node_id = query.value("lhs_node").toInt();
        trans_id = query.value("id").toInt();
        hash.insert(related_node_id, trans_id);
    }

    return hash;
}

void Sqlite::QueryTransList(TransList& trans_list, int node_id, QSqlQuery& query)
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
