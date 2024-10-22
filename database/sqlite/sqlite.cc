#include "sqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "global/resourcepool.h"
#include "global/sqlconnection.h"

Sqlite::Sqlite(CInfo& info, QObject* parent)
    : QObject(parent)
    , db_ { SqlConnection::Instance().Allocate(info.section) }
    , info_ { info }
{
}

Sqlite::~Sqlite() { qDeleteAll(trans_hash_); }

void Sqlite::RRemoveNode(int node_id)
{
    QMultiHash<int, int> node_trans { RemoveNodeFunction(node_id) };

    emit SFreeView(node_id);
    emit SRemoveNode(node_id);

    auto section { info_.section };
    if (section == Section::kFinance || section == Section::kProduct || section == Section::kTask) {
        emit SRemoveMultiTrans(node_trans);
        emit SUpdateMultiLeafTotal(node_trans.uniqueKeys());
    }

    // Recycle will mark all transactions for removal. This must occur after SRemoveMultiTrans()
    for (int trans_id : node_trans.values())
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
}

QMultiHash<int, int> Sqlite::RemoveNodeFunction(int node_id) const
{
    // finance, product, task
    const auto& const_trans_hash { std::as_const(trans_hash_) };
    QMultiHash<int, int> hash {};

    for (const auto* trans : const_trans_hash) {
        if (trans->lhs_node == node_id && trans->rhs_node != node_id)
            hash.emplace(trans->rhs_node, trans->id);

        if (trans->rhs_node == node_id && trans->lhs_node != node_id)
            hash.emplace(trans->lhs_node, trans->id);
    }

    return hash;
}

bool Sqlite::FreeView(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (const auto* trans : const_trans_hash) {
        if ((trans->lhs_node == old_node_id && trans->rhs_node == new_node_id) || (trans->rhs_node == old_node_id && trans->lhs_node == new_node_id)) {
            return false;
        }
    }

    return true;
}

bool Sqlite::RReplaceNode(int old_node_id, int new_node_id)
{
    // finance, product, task
    auto section { info_.section };
    if (section == Section::kPurchase || section == Section::kSales)
        return false;

    bool free { FreeView(old_node_id, new_node_id) };

    // begin deal with trans hash
    auto node_trans { ReplaceNodeFunction(old_node_id, new_node_id) };
    // end deal with trans hash

    if (node_trans.isEmpty())
        return true;

    // begin deal with database
    QSqlQuery query(*db_);
    CString& string { RReplaceNodeQS() };
    if (string.isEmpty())
        return false;

    query.prepare(string);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Failed in RReplaceNode" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SMoveMultiTrans(old_node_id, new_node_id, node_trans.values());
    emit SUpdateMultiLeafTotal(QList { old_node_id, new_node_id });

    if (section == Section::kProduct)
        emit SUpdateProductReference(old_node_id, new_node_id);

    // SFreeView will mark all referenced transactions for removal. This must occur after SMoveMultiTrans()
    if (free) {
        emit SFreeView(old_node_id);
        emit SRemoveNode(old_node_id);
    }

    return true;
}

bool Sqlite::RUpdateProductReference(int old_node_id, int new_node_id)
{
    CString& string { RUpdateProductReferenceQS() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.prepare(string);
    query.bindValue(":old_node_id", old_node_id);
    query.bindValue(":new_node_id", new_node_id);
    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RUpdateProductReference" << query.lastError().text();
        return false;
    }

    UpdateProductReference(old_node_id, new_node_id);

    return true;
}

bool Sqlite::RUpdateStakeholderReference(int old_node_id, int new_node_id)
{
    CString& string { RUpdateStakeholderReferenceQS() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.prepare(string);
    query.bindValue(":old_node_id", old_node_id);
    query.bindValue(":new_node_id", new_node_id);
    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RUpdateStakeholderReference" << query.lastError().text();
        return false;
    }

    UpdateStakeholderReference(old_node_id, new_node_id);
    return true;
}

bool Sqlite::ReadNode(NodeHash& node_hash)
{
    CString& string { ReadNodeQS() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNode" << query.lastError().text();
        return false;
    }

    Node* node {};
    while (query.next()) {
        node = ResourcePool<Node>::Instance().Allocate();
        ReadNodeQuery(node, query);
        node_hash.insert(node->id, node);
    }

    ReadRelationship(node_hash, query);
    return true;
}

bool Sqlite::WriteNode(int parent_id, Node* node) const
{
    // root_'s id is -1
    CString& string { WriteNodeQS() };
    if (string.isEmpty() || !node || node->id == -1)
        return false;

    QSqlQuery query(*db_);
    if (!DBTransaction([&]() {
            // 插入节点记录
            query.prepare(string);
            WriteNodeBind(node, query);

            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteNode" << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            // 插入节点路径记录
            WriteRelationship(node->id, parent_id, query);
            return true;
        })) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteNode commit";
        return false;
    }

    return true;
}

void Sqlite::CalculateLeafTotal(Node* node, QSqlQuery& query) const
{
    // finance, product, task
    bool rule { node->rule };
    int sign = rule ? 1 : -1;

    if (query.next()) {
        QVariant initial_balance { query.value("initial_balance") };
        QVariant final_balance { query.value("final_balance") };

        node->initial_total = sign * (initial_balance.isNull() ? 0.0 : initial_balance.toDouble());
        node->final_total = sign * (final_balance.isNull() ? 0.0 : final_balance.toDouble());
    }
}

bool Sqlite::LeafTotal(Node* node) const
{
    CString& string { LeafTotalQS() };

    if (string.isEmpty() || !node || node->id <= 0 || node->branch)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);
    query.bindValue(":node_id", node->id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in LeafTotal" << query.lastError().text();
        return false;
    }

    CalculateLeafTotal(node, query);
    return true;
}

QList<int> Sqlite::SearchNodeName(CString& text) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QString string {};
    if (text.isEmpty())
        string = QString("SELECT id FROM %1 WHERE removed = 0").arg(info_.node);
    else
        string = QString("SELECT id FROM %1 WHERE removed = 0 AND name LIKE :text").arg(info_.node);

    query.prepare(string);

    if (!text.isEmpty())
        query.bindValue(":text", "%" + text + "%");

    if (!query.exec()) {
        qWarning() << "Failed in SearchNodeName" << query.lastError().text();
        return {};
    }

    int node_id {};
    QList<int> node_list {};

    while (query.next()) {
        node_id = query.value("id").toInt();
        node_list.emplaceBack(node_id);
    }

    return node_list;
}

bool Sqlite::RemoveNode(int node_id, bool branch) const
{
    QSqlQuery query(*db_);

    CString& string_frist { RemoveNodeFirstQS() };

    QString string_second { RemoveNodeSecondQS() };
    if (branch)
        string_second = RemoveNodeBranchQS();

    CString& string_third { RemoveNodeThirdQS() };

    if (!DBTransaction([&]() {
            query.prepare(string_frist);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RemoveNode 1st" << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(string_second);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RemoveNode 2nd" << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(string_third);
            query.bindValue(":node_id", node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RemoveNode 3rd" << query.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed in RemoveNode";
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
                WHERE fp2.ancestor = :node_id AND fp2.descendant != :node_id AND fp1.ancestor != :node_id
            )
            UPDATE %1
            SET distance = distance - 1
            WHERE (ancestor, descendant) IN (
            SELECT ancestor, descendant FROM related_nodes)
            )")
        .arg(info_.path);
}

QString Sqlite::RemoveNodeThirdQS() const
{
    return QString("DELETE FROM %1 WHERE (descendant = :node_id OR ancestor = :node_id) AND distance !=0").arg(info_.path);
}

QString Sqlite::DragNodeFirstQS() const
{
    return QString(R"(
            WITH related_nodes AS (
                SELECT DISTINCT fp1.ancestor, fp2.descendant
                FROM %1 AS fp1
                INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
                WHERE fp2.ancestor = :node_id AND fp1.ancestor != :node_id
            )
            DELETE FROM %1
            WHERE (ancestor, descendant) IN (
            SELECT ancestor, descendant FROM related_nodes)
            )")
        .arg(info_.path);
}

QString Sqlite::DragNodeSecondQS() const
{
    return QString(R"(
            INSERT INTO %1 (ancestor, descendant, distance)
            SELECT fp1.ancestor, fp2.descendant, fp1.distance + fp2.distance + 1
            FROM %1 AS fp1
            INNER JOIN %1 AS fp2
            WHERE fp1.descendant = :destination_node_id AND fp2.ancestor = :node_id
            )")
        .arg(info_.path);
}

bool Sqlite::DragNode(int destination_node_id, int node_id) const
{
    QSqlQuery query(*db_);

    CString& string_first { DragNodeFirstQS() };
    CString& string_second { DragNodeSecondQS() };

    if (!DBTransaction([&]() {
            // 第一个查询
            query.prepare(string_first);
            query.bindValue(":node_id", node_id);

            if (!query.exec()) {
                qWarning() << "Failed in DragNode 1st" << query.lastError().text();
                return false;
            }

            query.clear();

            // 第二个查询
            query.prepare(string_second);
            query.bindValue(":node_id", node_id);
            query.bindValue(":destination_node_id", destination_node_id);

            if (!query.exec()) {
                qWarning() << "Failed in DragNode 2nd" << query.lastError().text();
                return false;
            }
            return true;
        })) {
        qWarning() << "Failed in DragNode";
        return false;
    }

    return true;
}

bool Sqlite::InternalReference(int node_id) const
{
    CString& string { InternalReferenceQS() };
    if (string.isEmpty() || node_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in InternalReference" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool Sqlite::ExternalReference(int node_id) const
{
    CString& string { ExternalReferenceQS() };
    if (string.isEmpty() || node_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ExternalReference" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool Sqlite::ReadTrans(TransShadowList& trans_shadow_list, int node_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString& string { ReadTransQS() };
    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadTrans" << query.lastError().text();
        return false;
    }

    ReadTransFunction(trans_shadow_list, node_id, query);
    return true;
}

void Sqlite::ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left) const
{
    trans_shadow->id = &trans->id;
    trans_shadow->state = &trans->state;
    trans_shadow->date_time = &trans->date_time;
    trans_shadow->code = &trans->code;
    trans_shadow->document = &trans->document;
    trans_shadow->description = &trans->description;
    trans_shadow->node_id = &trans->node_id;
    trans_shadow->discount_price = &trans->discount_price;
    trans_shadow->unit_price = &trans->unit_price;
    trans_shadow->settled = &trans->settled;

    trans_shadow->lhs_node = &(left ? trans->lhs_node : trans->rhs_node);
    trans_shadow->lhs_ratio = &(left ? trans->lhs_ratio : trans->rhs_ratio);
    trans_shadow->lhs_debit = &(left ? trans->lhs_debit : trans->rhs_debit);
    trans_shadow->lhs_credit = &(left ? trans->lhs_credit : trans->rhs_credit);

    trans_shadow->rhs_node = &(left ? trans->rhs_node : trans->lhs_node);
    trans_shadow->rhs_ratio = &(left ? trans->rhs_ratio : trans->lhs_ratio);
    trans_shadow->rhs_debit = &(left ? trans->rhs_debit : trans->lhs_debit);
    trans_shadow->rhs_credit = &(left ? trans->rhs_credit : trans->lhs_credit);
}

bool Sqlite::WriteTrans(TransShadow* trans_shadow)
{
    QSqlQuery query(*db_);
    CString& string { WriteTransQS() };

    query.prepare(string);
    WriteTransBind(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteTrans" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

bool Sqlite::WriteTransRange(const QList<TransShadow*>& list) const
{
    if (list.isEmpty())
        return false;

    QSqlQuery query(*db_);

    query.exec("PRAGMA synchronous = OFF");
    query.exec("PRAGMA journal_mode = MEMORY");

    if (!db_->transaction()) {
        qDebug() << "Failed to start transaction" << db_->lastError();
        return false;
    }

    CString& string { WriteTransQS() };

    // 插入多条记录的 SQL 语句
    query.prepare(string);

    const int size = list.size();
    QVariantList code_list {};
    QVariantList inside_product_list {};
    QVariantList unit_price_list {};
    QVariantList description_list {};
    QVariantList second_list {};
    QVariantList node_id_list {};
    QVariantList first_list {};
    QVariantList amount_list {};
    QVariantList discount_list {};
    QVariantList settled_list {};
    QVariantList outside_product_list {};
    QVariantList discount_price_list {};

    code_list.reserve(size);
    inside_product_list.reserve(size);
    unit_price_list.reserve(size);
    description_list.reserve(size);
    second_list.reserve(size);
    node_id_list.reserve(size);
    first_list.reserve(size);
    amount_list.reserve(size);
    discount_list.reserve(size);
    settled_list.reserve(size);
    outside_product_list.reserve(size);
    discount_price_list.reserve(size);

    // 遍历 list 并将每个字段的值添加到相应的 QVariantList 中
    for (const TransShadow* trans_shadow : list) {
        code_list.emplaceBack(*trans_shadow->code);
        inside_product_list.emplaceBack(*trans_shadow->lhs_node);
        unit_price_list.emplaceBack(*trans_shadow->unit_price);
        description_list.emplaceBack(*trans_shadow->description);
        second_list.emplaceBack(*trans_shadow->lhs_credit);
        node_id_list.emplaceBack(*trans_shadow->node_id);
        first_list.emplaceBack(*trans_shadow->lhs_debit);
        amount_list.emplaceBack(*trans_shadow->rhs_credit);
        discount_list.emplaceBack(*trans_shadow->rhs_debit);
        settled_list.emplaceBack(*trans_shadow->settled);
        outside_product_list.emplaceBack(*trans_shadow->rhs_node);
        discount_price_list.emplaceBack(*trans_shadow->discount_price);
    }

    // 批量绑定 QVariantList
    query.bindValue(":code", code_list);
    query.bindValue(":inside_product", inside_product_list);
    query.bindValue(":unit_price", unit_price_list);
    query.bindValue(":description", description_list);
    query.bindValue(":second", second_list);
    query.bindValue(":node_id", node_id_list);
    query.bindValue(":first", first_list);
    query.bindValue(":amount", amount_list);
    query.bindValue(":discount", discount_list);
    query.bindValue(":settled", settled_list);
    query.bindValue(":outside_product", outside_product_list);
    query.bindValue(":discount_price", discount_price_list);

    // 执行批量插入
    if (!query.execBatch()) {
        qDebug() << "Failed in WriteTransRange" << query.lastError();
        db_->rollback();
        return false;
    }

    // 提交事务
    if (!db_->commit()) {
        qDebug() << "Failed to commit transaction" << db_->lastError();
        db_->rollback();
        return false;
    }

    query.exec("PRAGMA synchronous = FULL");
    query.exec("PRAGMA journal_mode = DELETE");

    int last_id { query.lastInsertId().toInt() };

    for (auto i { list.size() - 1 }; i >= 0; --i) {
        *list.at(i)->id = last_id;
        --last_id;
    }

    return true;
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
        qWarning() << "Failed in RemoveTrans" << query.lastError().text();
        return false;
    }

    ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    return true;
}

bool Sqlite::UpdateNodeValue(const Node* node) const
{
    if (node->branch)
        return false;

    CString& string { UpdateNodeValueQS() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);

    query.prepare(string);
    UpdateNodeValueBind(node, query);

    if (!query.exec()) {
        qWarning() << "Failed in UpdateNodeValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::UpdateTransValue(const TransShadow* trans_shadow) const
{
    CString& string { UpdateTransValueQS() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);

    query.prepare(string);
    UpdateTransValueBind(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Failed in UpdateTransValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::UpdateField(CString& table, CVariant& value, CString& field, int id) const
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
        qWarning() << "Failed in UpdateField" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::UpdateCheckState(CString& column, CVariant& value, Check state) const
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
        qWarning() << "Failed in UpdateCheckState" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::SearchTrans(TransList& trans_list, CString& text) const
{
    if (text.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string { SearchTransQS() };
    query.prepare(string);

    query.bindValue(":text", text);
    query.bindValue(":description", "%" + text + "%");

    if (!query.exec()) {
        qWarning() << "Failed in Search Trans" << query.lastError().text();
        return false;
    }

    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        if (trans_hash_.contains(id)) {
            trans = trans_hash_.value(id);
            trans_list.emplaceBack(trans);
            continue;
        }

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans->id = id;

        ReadTransQuery(trans, query);
        trans_list.emplaceBack(trans);
    }

    return true;
}

bool Sqlite::ReadTransRange(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list)
{
    if (trans_id_list.empty() || node_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const qsizetype batch_size { 50 };
    const auto total_batches { (trans_id_list.size() + batch_size - 1) / batch_size };

    for (int batch_index = 0; batch_index != total_batches; ++batch_index) {
        int start = batch_index * batch_size;
        int end = std::min(start + batch_size, trans_id_list.size());

        QList<int> current_batch { trans_id_list.mid(start, end - start) };

        QStringList placeholder { current_batch.size(), "?" };
        QString string { ReadTransRangeQS(placeholder.join(",")) };

        query.prepare(string);

        for (int i = 0; i != current_batch.size(); ++i)
            query.bindValue(i, current_batch.at(i));

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadTransRange, batch" << batch_index << ": "
                       << query.lastError().text();
            continue;
        }

        ReadTransFunction(trans_shadow_list, node_id, query);
    }

    return true;
}

TransShadow* Sqlite::AllocateTransShadow()
{
    last_trans_ = ResourcePool<Trans>::Instance().Allocate();
    auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

    ConvertTrans(last_trans_, trans_shadow, true);
    return trans_shadow;
}

bool Sqlite::DBTransaction(std::function<bool()> function) const
{
    if (db_->transaction() && function() && db_->commit()) {
        return true;
    } else {
        db_->rollback();
        qWarning() << "Failed in Transaction";
        return false;
    }
}

bool Sqlite::ReadRelationship(const NodeHash& node_hash, QSqlQuery& query) const
{
    if (node_hash.isEmpty())
        return false;

    auto part = QString(R"(
    SELECT ancestor, descendant
    FROM %1
    WHERE distance = 1
)")
                    .arg(info_.path);

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Failed in ReadRelationship" << query.lastError().text();
        return false;
    }

    int ancestor_id {};
    int descendant_id {};
    Node* ancestor {};
    Node* descendant {};

    while (query.next()) {
        ancestor_id = query.value("ancestor").toInt();
        descendant_id = query.value("descendant").toInt();

        ancestor = node_hash.value(ancestor_id);
        descendant = node_hash.value(descendant_id);

        if (!ancestor || !descendant)
            continue;

        ancestor->children.emplaceBack(descendant);
        descendant->parent = ancestor;
    }

    return true;
}

bool Sqlite::WriteRelationship(int node_id, int parent_id, QSqlQuery& query) const
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
        qWarning() << "Failed in WriteRelationship" << query.lastError().text();
        return false;
    }

    return true;
}

void Sqlite::ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query)
{
    // finance, product, task
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

            ReadTransQuery(trans, query);
            trans_hash_.insert(id, trans);
        }

        ConvertTrans(trans, trans_shadow, node_id == trans->lhs_node);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

QMultiHash<int, int> Sqlite::ReplaceNodeFunction(int old_node_id, int new_node_id) const
{
    // finance, product, task
    const auto& const_trans_hash { std::as_const(trans_hash_) };
    QMultiHash<int, int> hash {};

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id && trans->rhs_node != new_node_id) {
            hash.emplace(trans->rhs_node, trans->id);
            trans->lhs_node = new_node_id;
        }

        if (trans->rhs_node == old_node_id && trans->lhs_node != new_node_id) {
            hash.emplace(trans->lhs_node, trans->id);
            trans->rhs_node = new_node_id;
        }
    }

    return hash;
}
