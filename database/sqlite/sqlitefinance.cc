#include "sqlitefinance.h"

SqliteFinance::SqliteFinance(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteFinance::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, initial_total, final_total
    FROM finance
    WHERE removed = 0
    )");
}

QString SqliteFinance::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO finance (name, code, description, note, node_rule, branch, unit)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit)
    )");
}

QString SqliteFinance::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqliteFinance::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::LeafTotalQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_debit AS debit, lhs_credit AS credit, lhs_ratio AS ratio FROM finance_transaction
    WHERE lhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT rhs_debit, rhs_credit, rhs_ratio FROM finance_transaction
    WHERE rhs_node = :node_id AND removed = 0
    )");
}

QString SqliteFinance::RRemoveNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqliteFinance::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO finance_transaction
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
    )");
}

QString SqliteFinance::BuildTransShadowListRangQS(CString& in_list) const
{
    return QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM finance_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

QString SqliteFinance::RelatedNodeTransQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_node, id FROM finance_transaction
    WHERE rhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT rhs_node, id FROM finance_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteFinance::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
    lhs_node = CASE
        WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id
        ELSE lhs_node
    END,
    rhs_node = CASE
        WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id
        ELSE rhs_node
    END
    WHERE lhs_node = :old_node_id OR rhs_node = :old_node_id;
    )");
}
