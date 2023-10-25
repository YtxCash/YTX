#include "tablemodeltask.h"

#include "global/resourcepool.h"

TableModelTask::TableModelTask(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
{
    AccumulateSubtotal(0, node_rule);
}

bool TableModelTask::UpdateDebit(Trans* trans, double value)
{
    double debit { *trans->debit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(debit - value) <= tolerance)
        return false;

    double credit { *trans->credit };
    double ratio { *trans->ratio };

    double abs { qAbs(value - credit) };
    *trans->debit = (value > credit) ? abs : 0;
    *trans->credit = (value <= credit) ? abs : 0;

    double t_debit { *trans->related_debit };
    double t_credit { *trans->related_credit };

    *trans->related_debit = *trans->credit;
    *trans->related_credit = *trans->debit;

    if (*trans->related_node == 0)
        return false;

    auto initial_debit_diff { *trans->debit - debit };
    auto initial_credit_diff { *trans->credit - credit };
    emit SUpdateOneTotal(*trans->node, initial_debit_diff, initial_credit_diff, initial_debit_diff * ratio, initial_credit_diff * ratio);

    auto t_initial_debit_diff { *trans->related_debit - t_debit };
    auto t_initial_credit_diff { *trans->related_credit - t_credit };
    emit SUpdateOneTotal(*trans->related_node, t_initial_debit_diff, t_initial_credit_diff, t_initial_debit_diff * ratio, t_initial_credit_diff * ratio);

    return true;
}

bool TableModelTask::UpdateCredit(Trans* trans, double value)
{
    double credit { *trans->credit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(credit - value) <= tolerance)
        return false;

    double debit { *trans->debit };

    double abs { qAbs(value - debit) };
    *trans->debit = (value > debit) ? 0 : abs;
    *trans->credit = (value <= debit) ? 0 : abs;

    double t_debit { *trans->related_debit };
    double t_credit { *trans->related_credit };

    *trans->related_debit = *trans->credit;
    *trans->related_credit = *trans->debit;

    if (*trans->related_node == 0)
        return false;

    auto initial_debit_diff { *trans->debit - debit };
    auto initial_credit_diff { *trans->credit - credit };
    emit SUpdateOneTotal(*trans->node, initial_debit_diff, initial_credit_diff, initial_debit_diff, initial_credit_diff);

    auto t_initial_debit_diff { *trans->related_debit - t_debit };
    auto t_initial_credit_diff { *trans->related_credit - t_credit };
    emit SUpdateOneTotal(*trans->related_node, t_initial_debit_diff, t_initial_credit_diff, t_initial_debit_diff, t_initial_credit_diff);

    return true;
}

bool TableModelTask::UpdateRatio(Trans* trans, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };
    double ratio { *trans->ratio };

    if (std::abs(ratio - value) <= tolerance || value <= 0)
        return false;

    *trans->ratio = value;
    return true;
}
