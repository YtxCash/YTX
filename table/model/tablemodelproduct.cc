#include "tablemodelproduct.h"

TableModelProduct::TableModelProduct(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
{
    AccumulateSubtotal(0, node_rule);
}

bool TableModelProduct::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double debit { *trans_shadow->debit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(debit - value) < tolerance)
        return false;

    double credit { *trans_shadow->credit };

    double abs { qAbs(value - credit) };
    *trans_shadow->debit = (value > credit) ? abs : 0;
    *trans_shadow->credit = (value <= credit) ? abs : 0;

    *trans_shadow->related_debit = *trans_shadow->credit;
    *trans_shadow->related_credit = *trans_shadow->debit;

    if (*trans_shadow->related_node == 0)
        return false;

    auto unit_cost { *trans_shadow->ratio };
    auto quantity_debit_diff { *trans_shadow->debit - debit };
    auto quantity_credit_diff { *trans_shadow->credit - credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateOneTotal(*trans_shadow->node, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateOneTotal(*trans_shadow->related_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

    return true;
}

bool TableModelProduct::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double credit { *trans_shadow->credit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(credit - value) < tolerance)
        return false;

    double debit { *trans_shadow->debit };

    double abs { qAbs(value - debit) };
    *trans_shadow->debit = (value > debit) ? 0 : abs;
    *trans_shadow->credit = (value <= debit) ? 0 : abs;

    *trans_shadow->related_debit = *trans_shadow->credit;
    *trans_shadow->related_credit = *trans_shadow->debit;

    if (*trans_shadow->related_node == 0)
        return false;

    auto unit_cost { *trans_shadow->ratio };
    auto quantity_debit_diff { *trans_shadow->debit - debit };
    auto quantity_credit_diff { *trans_shadow->credit - credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateOneTotal(*trans_shadow->node, amount_debit_diff, amount_credit_diff, quantity_debit_diff, quantity_credit_diff);
    emit SUpdateOneTotal(*trans_shadow->related_node, amount_credit_diff, amount_debit_diff, quantity_credit_diff, quantity_debit_diff);

    return true;
}

bool TableModelProduct::UpdateRatio(TransShadow* trans_shadow, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };
    double ratio { *trans_shadow->ratio };

    if (std::abs(ratio - value) < tolerance || value < 0)
        return false;

    auto result { value - ratio };
    *trans_shadow->ratio = value;
    *trans_shadow->related_ratio = value;

    if (*trans_shadow->related_node == 0)
        return false;

    emit SUpdateOneTotal(*trans_shadow->node, 0, 0, *trans_shadow->debit * result, *trans_shadow->credit * result);
    emit SUpdateOneTotal(*trans_shadow->related_node, 0, 0, *trans_shadow->related_debit * result, *trans_shadow->related_credit * result);

    return true;
}
