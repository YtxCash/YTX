#include "sqlitetask.h"

SqliteTask::SqliteTask(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteTask::BuildTreeQS() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, node_rule, branch, unit, initial_total, final_total
    FROM task
    WHERE removed = 0
    )");
}

QString SqliteTask::InsertNodeQS() const
{
    return QStringLiteral(R"(
    INSERT INTO task (name, code, description, note, node_rule, branch, unit)
    VALUES (:name, :code, :description, :note, :node_rule, :branch, :unit)
    )");
}

QString SqliteTask::RemoveNodeSecondQS() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqliteTask::InternalReferenceQS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM task_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteTask::LeafTotalQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_debit AS debit, lhs_credit AS credit, lhs_ratio AS ratio FROM task_transaction
    WHERE lhs_node = (:node_id) AND removed = 0
    UNION ALL
    SELECT rhs_debit, rhs_credit, rhs_ratio FROM task_transaction
    WHERE rhs_node = (:node_id) AND removed = 0
    )");
}

QString SqliteTask::RRemoveNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction
    SET removed = 1
    WHERE lhs_node = :node_id OR rhs_node = :node_id
    )");
}

QString SqliteTask::BuildTransShadowListQS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM task_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteTask::InsertTransShadowQS() const
{
    return QStringLiteral(R"(
    INSERT INTO task_transaction
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :code, :document)
    )");
}

QString SqliteTask::BuildTransShadowListRangQS(CString& in_list) const
{
    return QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, code, document, date_time
    FROM task_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

QString SqliteTask::RelatedNodeTransQS() const
{
    return QStringLiteral(R"(
    SELECT lhs_node, id FROM task_transaction
    WHERE rhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT rhs_node, id FROM task_transaction
    WHERE lhs_node = :node_id AND removed = 0
    )");
}

QString SqliteTask::RReplaceNodeQS() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction SET
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
