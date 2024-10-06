#include "tablemodeltask.h"

#include "component/constvalue.h"

TableModelTask::TableModelTask(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
}

QVariant TableModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
        return *trans_shadow->id;
    case TableEnum::kDateTime:
        return *trans_shadow->date_time;
    case TableEnum::kCode:
        return *trans_shadow->code;
    case TableEnum::kLhsRatio:
        return *trans_shadow->unit_price;
    case TableEnum::kDescription:
        return *trans_shadow->description;
    case TableEnum::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnum::kState:
        return *trans_shadow->state;
    case TableEnum::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : QString::number(trans_shadow->document->size());
    case TableEnum::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnum::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnum::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

void TableModelTask::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.part_table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnum kColumn { column };

        switch (kColumn) {
        case TableEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnum::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnum::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        case TableEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnum::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnum::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnum::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnum::kCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();

    AccumulateSubtotal(0, rule_);
}

bool TableModelTask::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double lhs_debit { *trans_shadow->lhs_debit };
    if (std::abs(lhs_debit - value) < TOLERANCE)
        return false;

    double lhs_credit { *trans_shadow->lhs_credit };
    double lhs_ratio { *trans_shadow->lhs_ratio };

    double abs { qAbs(value - lhs_credit) };
    *trans_shadow->lhs_debit = (value > lhs_credit) ? abs : 0;
    *trans_shadow->lhs_credit = (value <= lhs_credit) ? abs : 0;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };

    *trans_shadow->rhs_debit = *trans_shadow->lhs_credit;
    *trans_shadow->rhs_credit = *trans_shadow->lhs_debit;

    if (*trans_shadow->rhs_node == 0)
        return false;

    auto lhs_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    auto lhs_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafTotal(*trans_shadow->lhs_node, lhs_debit_diff, lhs_credit_diff, lhs_debit_diff * lhs_ratio, lhs_credit_diff * lhs_ratio);

    auto rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    auto rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafTotal(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * lhs_ratio, rhs_credit_diff * lhs_ratio);

    return true;
}

bool TableModelTask::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double lhs_credit { *trans_shadow->lhs_credit };
    if (std::abs(lhs_credit - value) < TOLERANCE)
        return false;

    double lhs_debit { *trans_shadow->lhs_debit };

    double abs { qAbs(value - lhs_debit) };
    *trans_shadow->lhs_debit = (value > lhs_debit) ? 0 : abs;
    *trans_shadow->lhs_credit = (value <= lhs_debit) ? 0 : abs;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };

    *trans_shadow->rhs_debit = *trans_shadow->lhs_credit;
    *trans_shadow->rhs_credit = *trans_shadow->lhs_debit;

    if (*trans_shadow->rhs_node == 0)
        return false;

    auto lhs_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    auto lhs_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafTotal(*trans_shadow->lhs_node, lhs_debit_diff, lhs_credit_diff, lhs_debit_diff, lhs_credit_diff);

    auto rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    auto rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafTotal(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff, rhs_credit_diff);

    return true;
}

bool TableModelTask::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double unit_cost { *trans_shadow->unit_price };
    if (std::abs(unit_cost - value) < TOLERANCE || value < 0)
        return false;

    auto result { value - unit_cost };
    *trans_shadow->unit_price = value;

    if (*trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateField(info_.transaction, value, UNIT_COST, *trans_shadow->id);

    emit SUpdateLeafTotal(*trans_shadow->lhs_node, 0, 0, *trans_shadow->lhs_debit * result, *trans_shadow->lhs_credit * result);
    emit SUpdateLeafTotal(*trans_shadow->rhs_node, 0, 0, *trans_shadow->rhs_debit * result, *trans_shadow->rhs_credit * result);

    return true;
}
