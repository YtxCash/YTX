#include "sqlitestakeholder.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteStakeholder::SqliteStakeholder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

bool SqliteStakeholder::RReplaceNode(int old_node_id, int new_node_id)
{
    // begin deal with trans hash
    auto trans_id_list { DialogReplaceNode(old_node_id, new_node_id) };
    // end deal with trans hash

    // begin deal with database
    QSqlQuery query(*db_);
    CString& string { RReplaceNodeQS() };

    query.prepare(string);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Error in replace node setp" << query.lastError().text();
        return false;
    }
    // end deal with database

    emit SMoveMultiTrans(0, new_node_id, trans_id_list);
    emit SUpdateStakeholderReference(old_node_id, new_node_id);

    // SFreeView will mark all referenced transactions for removal. This must occur after SMoveMultiTrans()
    emit SFreeView(old_node_id);
    emit SRemoveNode(old_node_id);

    return true;
}

bool SqliteStakeholder::RRemoveNode(int node_id)
{
    emit SFreeView(node_id);
    emit SRemoveNode(node_id);
    return true;
}

QString SqliteStakeholder::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, employee, deadline, payment_period, tax_rate
    FROM stakeholder
    WHERE removed = 0
    )");
}

QString SqliteStakeholder::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder (name, code, description, note, rule, branch, unit, employee, deadline, payment_period, tax_rate)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :employee, :deadline, :payment_period, :tax_rate)
    )");
}

void SqliteStakeholder::WriteNode(Node* node, QSqlQuery& query)
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":rule", node->rule);
    query.bindValue(":branch", node->branch);
    query.bindValue(":unit", node->unit);
    query.bindValue(":employee", node->employee);
    query.bindValue(":deadline", node->party);
    query.bindValue(":payment_period", node->first);
    query.bindValue(":tax_rate", node->second);
}

QString SqliteStakeholder::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET removed = 1
    WHERE lhs_node = :node_id
    )");
}

QString SqliteStakeholder::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM stakeholder_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::ExternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM sales WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM purchase WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM sales_transaction WHERE rhs_node = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM purchase_transaction WHERE rhs_node = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteStakeholder::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node
    FROM stakeholder_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder_transaction
    (date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node)
    VALUES
    (:date_time, :code, :lhs_node, :lhs_ratio, :description, :document, :state, :rhs_node)
    )");
}

QString SqliteStakeholder::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET lhs_node = :new_node_id
    WHERE lhs_node = :old_node_id
    )");
}

QString SqliteStakeholder::RUpdateProductReferenceQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET rhs_node = :new_node_id
    WHERE rhs_node = :old_node_id
    )");
}

QList<int> SqliteStakeholder::DialogReplaceNode(int old_node_id, int new_node_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };
    QList<int> list {};

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id) {
            list.emplaceBack(trans->id);
            trans->lhs_node = new_node_id;
        }
    }

    return list;
}

void SqliteStakeholder::WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query)
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":lhs_node", *trans_shadow->node);
    query.bindValue(":lhs_ratio", *trans_shadow->ratio);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":rhs_node", *trans_shadow->related_node);
}

void SqliteStakeholder::UpdateProductReference(int old_node_id, int new_node_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash)
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
}

void SqliteStakeholder::QueryTransShadowList(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        trans->id = id;

        ReadTrans(trans, query);
        trans_hash_.insert(id, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

QString SqliteStakeholder::BuildTransShadowListRangQS(CString& in_list) const
{
    return QString(R"(
    SELECT id, date_time, code, lhs_node, lhs_ratio, description, document, state, rhs_node
    FROM stakeholder_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

void SqliteStakeholder::ReadNode(Node* node, const QSqlQuery& query)
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->rule = query.value("rule").toBool();
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
