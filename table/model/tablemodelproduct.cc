#include "tablemodelproduct.h"

TableModelProduct::TableModelProduct(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
{
    AccumulateSubtotal(0, node_rule);
}

bool TableModelProduct::UpdateDebit(Trans* trans, double value)
{
    double debit { *trans->debit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(debit - value) < tolerance)
        return false;

    double credit { *trans->credit };

    double abs { qAbs(value - credit) };
    *trans->debit = (value > credit) ? abs : 0;
    *trans->credit = (value <= credit) ? abs : 0;

    *trans->related_debit = *trans->credit;
    *trans->related_credit = *trans->debit;

    if (*trans->related_node == 0)
        return false;

    auto unit_cost { *trans->ratio };
    auto quantity_debit_diff { *trans->debit - debit };
    auto quantity_credit_diff { *trans->credit - credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateOneTotal(*trans->node, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateOneTotal(*trans->related_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

    return true;
}

bool TableModelProduct::UpdateCredit(Trans* trans, double value)
{
    double credit { *trans->credit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(credit - value) < tolerance)
        return false;

    double debit { *trans->debit };

    double abs { qAbs(value - debit) };
    *trans->debit = (value > debit) ? 0 : abs;
    *trans->credit = (value <= debit) ? 0 : abs;

    *trans->related_debit = *trans->credit;
    *trans->related_credit = *trans->debit;

    if (*trans->related_node == 0)
        return false;

    auto unit_cost { *trans->ratio };
    auto quantity_debit_diff { *trans->debit - debit };
    auto quantity_credit_diff { *trans->credit - credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateOneTotal(*trans->node, amount_debit_diff, amount_credit_diff, quantity_debit_diff, quantity_credit_diff);
    emit SUpdateOneTotal(*trans->related_node, amount_credit_diff, amount_debit_diff, quantity_credit_diff, quantity_debit_diff);

    return true;
}

bool TableModelProduct::UpdateRatio(Trans* trans, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };
    double ratio { *trans->ratio };

    if (std::abs(ratio - value) < tolerance || value < 0)
        return false;

    auto result { value - ratio };
    *trans->ratio = value;
    *trans->related_ratio = value;

    if (*trans->related_node == 0)
        return false;

    emit SUpdateOneTotal(*trans->node, 0, 0, *trans->debit * result, *trans->credit * result);
    emit SUpdateOneTotal(*trans->related_node, 0, 0, *trans->related_debit * result, *trans->related_credit * result);

    return true;
}
