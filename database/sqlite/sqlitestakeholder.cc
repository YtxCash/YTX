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

void SqliteStakeholder::RRemoveNode(int node_id)
{
    emit SFreeView(node_id);
    emit SRemoveNode(node_id);
}

QString SqliteStakeholder::ReadNodeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, employee, deadline, payment_period, tax_rate
    FROM stakeholder
    WHERE removed = 0
    )");
}

QString SqliteStakeholder::WriteNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder (name, code, description, note, rule, branch, unit, employee, deadline, payment_period, tax_rate)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :employee, :deadline, :payment_period, :tax_rate)
    )");
}

void SqliteStakeholder::WriteNodeBind(Node* node, QSqlQuery& query)
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
    WHERE node = :node_id
    )");
}

QString SqliteStakeholder::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM stakeholder_transaction
    WHERE node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::ExternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM sales WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM purchase WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM sales_transaction WHERE outside_product = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM purchase_transaction WHERE outside_product = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteStakeholder::ReadTransQS() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE node = :node_id AND removed = 0
    )");
}

QString SqliteStakeholder::WriteTransQS() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder_transaction
    (date_time, code, node, unit_price, description, document, state, inside_product)
    VALUES
    (:date_time, :code, :node, :unit_price, :description, :document, :state, :inside_product)
    )");
}

QString SqliteStakeholder::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET node = :new_node_id
    WHERE node = :old_node_id
    )");
}

QString SqliteStakeholder::RUpdateProductReferenceQS() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )");
}

QString SqliteStakeholder::SearchTransQS() const
{
    return QStringLiteral(R"(
    SELECT  id, date_time, code, node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE (unit_price = :text OR description LIKE :description) AND removed = 0
    ORDER BY date_time
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

void SqliteStakeholder::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query)
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":node", *trans_shadow->lhs_node);
    query.bindValue(":unit_price", *trans_shadow->unit_price);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":inside_product", *trans_shadow->rhs_node);
}

void SqliteStakeholder::UpdateProductReference(int old_node_id, int new_node_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash)
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
}

void SqliteStakeholder::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value("id").toInt();

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        trans->id = id;

        ReadTransQuery(trans, query);
        trans_hash_.insert(id, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

QString SqliteStakeholder::ReadTransRangeQS(CString& in_list) const
{
    return QString(R"(
    SELECT id, date_time, code, node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

void SqliteStakeholder::ReadNodeQuery(Node* node, const QSqlQuery& query)
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

void SqliteStakeholder::ReadTransQuery(Trans* trans, const QSqlQuery& query)
{
    trans->lhs_node = query.value("node").toInt();
    trans->rhs_node = query.value("inside_product").toInt();
    trans->unit_price = query.value("unit_price").toDouble();
    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->state = query.value("state").toBool();
    trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
}
