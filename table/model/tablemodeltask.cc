#include "tablemodeltask.h"

TableModelTask::TableModelTask(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
}

bool TableModelTask::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double debit { *trans_shadow->debit };
    if (std::abs(debit - value) < TOLERANCE)
        return false;

    double credit { *trans_shadow->credit };
    double ratio { *trans_shadow->ratio };

    double abs { qAbs(value - credit) };
    *trans_shadow->debit = (value > credit) ? abs : 0;
    *trans_shadow->credit = (value <= credit) ? abs : 0;

    double t_debit { *trans_shadow->related_debit };
    double t_credit { *trans_shadow->related_credit };

    *trans_shadow->related_debit = *trans_shadow->credit;
    *trans_shadow->related_credit = *trans_shadow->debit;

    if (*trans_shadow->related_node == 0)
        return false;

    auto initial_debit_diff { *trans_shadow->debit - debit };
    auto initial_credit_diff { *trans_shadow->credit - credit };
    emit SUpdateLeafTotal(*trans_shadow->node, initial_debit_diff, initial_credit_diff, initial_debit_diff * ratio, initial_credit_diff * ratio);

    auto t_initial_debit_diff { *trans_shadow->related_debit - t_debit };
    auto t_initial_credit_diff { *trans_shadow->related_credit - t_credit };
    emit SUpdateLeafTotal(*trans_shadow->related_node, t_initial_debit_diff, t_initial_credit_diff, t_initial_debit_diff * ratio, t_initial_credit_diff * ratio);

    return true;
}

bool TableModelTask::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double credit { *trans_shadow->credit };
    if (std::abs(credit - value) < TOLERANCE)
        return false;

    double debit { *trans_shadow->debit };

    double abs { qAbs(value - debit) };
    *trans_shadow->debit = (value > debit) ? 0 : abs;
    *trans_shadow->credit = (value <= debit) ? 0 : abs;

    double t_debit { *trans_shadow->related_debit };
    double t_credit { *trans_shadow->related_credit };

    *trans_shadow->related_debit = *trans_shadow->credit;
    *trans_shadow->related_credit = *trans_shadow->debit;

    if (*trans_shadow->related_node == 0)
        return false;

    auto initial_debit_diff { *trans_shadow->debit - debit };
    auto initial_credit_diff { *trans_shadow->credit - credit };
    emit SUpdateLeafTotal(*trans_shadow->node, initial_debit_diff, initial_credit_diff, initial_debit_diff, initial_credit_diff);

    auto t_initial_debit_diff { *trans_shadow->related_debit - t_debit };
    auto t_initial_credit_diff { *trans_shadow->related_credit - t_credit };
    emit SUpdateLeafTotal(*trans_shadow->related_node, t_initial_debit_diff, t_initial_credit_diff, t_initial_debit_diff, t_initial_credit_diff);

    return true;
}

bool TableModelTask::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double ratio { *trans_shadow->ratio };
    if (std::abs(ratio - value) < TOLERANCE || value <= 0)
        return false;

    *trans_shadow->ratio = value;
    return true;
}
