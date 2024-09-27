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

Sqlite::~Sqlite() { qDeleteAll(trans_hash_); }

bool Sqlite::RRemoveNode(int node_id)
{
    auto node_trans { RelatedNodeTrans(node_id) };
    auto trans_id_list { node_trans.values() };

    // begin deal with database
    QSqlQuery query(*db_);
    const auto& string { RRemoveNodeQS() };

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed to remove multi in transaction table" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SFreeView(node_id);
    emit SRemoveNode(node_id);
    emit SRemoveMulti(node_trans);
    emit SUpdateMultiTotal(node_trans.uniqueKeys());

    // begin deal with trans hash
    for (int trans_id : trans_id_list)
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    // end deal with trans hash

    return true;
}

bool Sqlite::RReplaceNode(int old_node_id, int new_node_id)
{
    auto node_trans { RelatedNodeTrans(old_node_id) };
    bool free { !node_trans.contains(new_node_id) };

    node_trans.remove(new_node_id);

    // begin deal with trans hash
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto& trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id && trans->rhs_node != new_node_id)
            trans->lhs_node = new_node_id;

        if (trans->rhs_node == old_node_id && trans->lhs_node != new_node_id)
            trans->rhs_node = new_node_id;
    }
    // end deal with trans hash

    // begin deal with database
    QSqlQuery query(*db_);
    const auto& string { RRemoveNodeQS() };

    query.prepare(string);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Error in replace node setp" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SMoveMulti(info_.section, old_node_id, new_node_id, node_trans.values());

    if (free) {
        emit SFreeView(old_node_id);
        emit SRemoveNode(old_node_id);
    }

    if (info_.section != Section::kStakeholder)
        emit SUpdateMultiTotal(QList { old_node_id, new_node_id });

    if (info_.section == Section::kProduct)
        emit SUpdateProductReference(old_node_id, new_node_id);

    return true;
}

bool Sqlite::RUpdateProductReference(int old_node_id, int new_node_id)
{
    QSqlQuery query(*db_);
    const auto& string { RUpdateProductReferenceQS() };
    if (string.isEmpty())
        return false;

    query.prepare(string);
    query.bindValue(":old_node_id", old_node_id);
    query.bindValue(":new_node_id", new_node_id);
    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "error in RRelpaceReference." << query.lastError().text();
        return false;
    }

    UpdateProductReference(old_node_id, new_node_id);

    return true;
}

bool Sqlite::BuildTree(NodeHash& node_hash)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const QString& string { BuildTreeQS() };
    query.prepare(string);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Error in build tree." << query.lastError().text();
        return false;
    }

    BuildNodeHash(node_hash, query);
    query.clear();
    ReadRelationship(node_hash, query);
    return true;
}

bool Sqlite::InsertNode(int parent_id, Node* node)
{ // root_'s id is -1
    if (!node || node->id == -1)
        return false;

    QSqlQuery query(*db_);
    const QString& string { InsertNodeQS() };

    if (!DBTransaction([&]() {
            // 插入节点记录
            query.prepare(string);
            WriteNode(node, query);

            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed to insert node." << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            WriteRelationship(node->id, parent_id, query);
            return true;
        })) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed to commit insert node.";
        return false;
    }

    return true;
}

void Sqlite::WriteNode(Node* node, QSqlQuery& query)
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":node_rule", node->node_rule);
    query.bindValue(":branch", node->branch);
    query.bindValue(":unit", node->unit);
}

void Sqlite::CalculateLeafTotal(Node* node, QSqlQuery& query)
{
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

void Sqlite::LeafTotal(Node* node)
{
    if (!node || node->id <= 0 || node->branch)
        return;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const auto& string { LeafTotalQS() };
    if (string.isEmpty())
        return;

    query.prepare(string);
    query.bindValue(":node_id", node->id);
    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Error in calculate leaf total." << query.lastError().text();
        return;
    }

    CalculateLeafTotal(node, query);
}

bool Sqlite::RemoveNode(int node_id, bool branch)
{
    QSqlQuery query(*db_);

    const QString string_frist { RemoveNodeFirstQS() };
    QString string_second { RemoveNodeSecondQS() };

    if (branch)
        string_second = RemoveNodeBranchQS();

    //     auto part_22nd = QString("UPDATE %1 "
    //                                   "SET distance = distance - 1 "
    //                                   "WHERE (descendant IN (SELECT descendant
    //                                   FROM %1 " "WHERE ancestor = :node_id AND
    //                                   ancestor != descendant) AND ancestor IN
    //                                   (SELECT ancestor FROM %1 " "WHERE
    //                                   descendant = :node_id AND ancestor !=
    //                                   descendant)) ")
    //                               .arg(info_.path);

    const QString string_third { RemoveNodeThirdQS() };

    if (!DBTransaction([&]() {
            query.prepare(string_frist);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node record 1st step: " << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(string_second);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section)
                           << "Failed to remove node_transaction record 2nd step: " << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(string_third);
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

QString Sqlite::RemoveNodeFirstQS() const
{
    return QString(R"(
            UPDATE %1
            SET removed = 1
            WHERE id = :node_id
            )")
        .arg(info_.node);
}

QString Sqlite::RemoveNodeBranchQS() const
{
    return QString(R"(
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

QString Sqlite::RemoveNodeThirdQS() const
{
    return QString("DELETE FROM %1 WHERE (descendant = :node_id OR ancestor = :node_id) AND distance !=0").arg(info_.path);
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

bool Sqlite::InternalReference(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const auto& string { InternalReferenceQS() };
    if (string.isEmpty() || node_id <= 0)
        return false;

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed to count internal reference times." << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool Sqlite::ExternalReference(int node_id) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const auto& string { ExternalReferenceQS() };
    if (string.isEmpty() || node_id <= 0)
        return false;

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed to count external reference times." << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

void Sqlite::BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const auto& string { BuildTransShadowListQS() };

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Error in Construct Table" << query.lastError().text();
        return;
    }

    QueryTransShadowList(trans_shadow_list, node_id, query);
}

void Sqlite::ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left)
{
    trans_shadow->id = &trans->id;
    trans_shadow->state = &trans->state;
    trans_shadow->date_time = &trans->date_time;
    trans_shadow->code = &trans->code;
    trans_shadow->document = &trans->document;
    trans_shadow->description = &trans->description;

    trans_shadow->node = &(left ? trans->lhs_node : trans->rhs_node);
    trans_shadow->ratio = &(left ? trans->lhs_ratio : trans->rhs_ratio);
    trans_shadow->debit = &(left ? trans->lhs_debit : trans->rhs_debit);
    trans_shadow->credit = &(left ? trans->lhs_credit : trans->rhs_credit);

    trans_shadow->related_node = &(left ? trans->rhs_node : trans->lhs_node);
    trans_shadow->related_ratio = &(left ? trans->rhs_ratio : trans->lhs_ratio);
    trans_shadow->related_debit = &(left ? trans->rhs_debit : trans->lhs_debit);
    trans_shadow->related_credit = &(left ? trans->rhs_credit : trans->lhs_credit);
}

void Sqlite::ReadTrans(Trans* trans, const QSqlQuery& query)
{
    trans->lhs_node = query.value("lhs_node").toInt();
    trans->lhs_ratio = query.value("lhs_ratio").toDouble();
    trans->lhs_debit = query.value("lhs_debit").toDouble();
    trans->lhs_credit = query.value("lhs_credit").toDouble();

    trans->rhs_node = query.value("rhs_node").toInt();
    trans->rhs_ratio = query.value("rhs_ratio").toDouble();
    trans->rhs_debit = query.value("rhs_debit").toDouble();
    trans->rhs_credit = query.value("rhs_credit").toDouble();

    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
    trans->state = query.value("state").toBool();
}

bool Sqlite::InsertTransShadow(TransShadow* trans_shadow)
{
    QSqlQuery query(*db_);
    const auto& string { InsertTransShadowQS() };

    query.prepare(string);
    WriteTransShadow(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed to insert record in transaction table" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

void Sqlite::WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query)
{
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
}

bool Sqlite::RemoveTrans(int trans_id)
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

    ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    return true;
}

bool Sqlite::UpdateTrans(int trans_id)
{
    QSqlQuery query(*db_);
    auto trans { trans_hash_.value(trans_id) };

    auto part = QString(R"(
    UPDATE %1 SET
    lhs_node = :lhs_node, lhs_ratio = :lhs_ratio, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
    rhs_node = :rhs_node, rhs_ratio = :rhs_ratio, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :id
)")
                    .arg(info_.transaction);

    query.prepare(part);
    query.bindValue(":lhs_node", trans->lhs_node);
    query.bindValue(":lhs_ratio", trans->lhs_ratio);
    query.bindValue(":lhs_debit", trans->lhs_debit);
    query.bindValue(":lhs_credit", trans->lhs_credit);
    query.bindValue(":rhs_node", trans->rhs_node);
    query.bindValue(":rhs_ratio", trans->rhs_ratio);
    query.bindValue(":rhs_debit", trans->rhs_debit);
    query.bindValue(":rhs_credit", trans->rhs_credit);
    query.bindValue(":id", trans_id);

    if (!query.exec()) {
        qWarning() << "Failed to update record in transaction table 1st " << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::UpdateField(CString& table, CVariant& value, CString& field, int id)
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
    query.bindValue(":value", value);

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

void Sqlite::BuildTransShadowList(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QStringList list {};

    for (const auto& id : trans_id_list)
        list.append(QString::number(id));

    auto part { BuildTransShadowListRangQS(list) };

    query.prepare(part);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Error in ConstructTable 1st" << query.lastError().text();
        return;
    }

    QueryTransShadowList(trans_shadow_list, node_id, query);
}

TransShadow* Sqlite::AllocateTransShadow()
{
    last_trans_ = ResourcePool<Trans>::Instance().Allocate();
    auto trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

    ConvertTrans(last_trans_, trans_shadow, true);
    return trans_shadow;
}

void Sqlite::BuildNodeHash(NodeHash& node_hash, QSqlQuery& query)
{
    Node* node {};

    while (query.next()) {
        node = ResourcePool<Node>::Instance().Allocate();
        ReadNode(node, query);
        node_hash.insert(node->id, node);
    }
}

void Sqlite::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->node_rule = query.value("node_rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->initial_total = query.value("initial_total").toDouble();
    node->final_total = query.value("final_total").toDouble();
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

void Sqlite::ReadRelationship(const NodeHash& node_hash, QSqlQuery& query)
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

void Sqlite::WriteRelationship(int node_id, int parent_id, QSqlQuery& query)
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

    const auto& string { RelatedNodeTransQS() };

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Error in RelatedNodeTrans" << query.lastError().text();
        return {};
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

void Sqlite::QueryTransShadowList(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        if (trans_hash_.contains(id)) {
            trans = trans_hash_.value(id);
        } else {
            trans = ResourcePool<Trans>::Instance().Allocate();
            trans->id = id;

            ReadTrans(trans, query);
            trans_hash_.insert(id, trans);
        }

        ConvertTrans(trans, trans_shadow, node_id == trans->lhs_node);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}
