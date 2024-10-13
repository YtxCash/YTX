#include "tablemodeltask.h"

#include "component/constvalue.h"
#include "global/resourcepool.h"

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
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
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
    if (column <= -1 || column >= info_.table_header.size() - 1)
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

    RunAccumulateSubtotal(0, rule_);
}

bool TableModelTask::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        auto unit_cost { *trans_shadow->unit_price };
        auto debit { *trans_shadow->lhs_debit };
        auto credit { *trans_shadow->lhs_credit };
        emit SUpdateLeafValue(node_id_, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SUpdateLeafValue(*trans_shadow->rhs_node, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, rhs_node_id, trans_id);

        RunAccumulateSubtotal(row, rule_);
        sql_->RemoveTrans(trans_id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelTask::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double lhs_debit { *trans_shadow->lhs_debit };
    if (std::abs(lhs_debit - value) < TOLERANCE)
        return false;

    double lhs_credit { *trans_shadow->lhs_credit };

    double abs { qAbs(value - lhs_credit) };
    *trans_shadow->lhs_debit = (value > lhs_credit) ? abs : 0;
    *trans_shadow->lhs_credit = (value <= lhs_credit) ? abs : 0;

    *trans_shadow->rhs_debit = *trans_shadow->lhs_credit;
    *trans_shadow->rhs_credit = *trans_shadow->lhs_debit;

    if (*trans_shadow->rhs_node == 0)
        return false;

    auto unit_cost { *trans_shadow->unit_price };
    auto quantity_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    auto quantity_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateLeafValue(*trans_shadow->lhs_node, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

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

    *trans_shadow->rhs_debit = *trans_shadow->lhs_credit;
    *trans_shadow->rhs_credit = *trans_shadow->lhs_debit;

    if (*trans_shadow->rhs_node == 0)
        return false;

    auto unit_cost { *trans_shadow->unit_price };
    auto quantity_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    auto quantity_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateLeafValue(*trans_shadow->lhs_node, amount_debit_diff, amount_credit_diff, quantity_debit_diff, quantity_credit_diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, amount_credit_diff, amount_debit_diff, quantity_credit_diff, quantity_debit_diff);

    return true;
}

bool TableModelTask::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double unit_cost { *trans_shadow->unit_price };
    if (std::abs(unit_cost - value) < TOLERANCE || value < 0)
        return false;

    auto diff { value - unit_cost };
    *trans_shadow->unit_price = value;

    if (*trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateField(info_.transaction, value, UNIT_COST, *trans_shadow->id);

    emit SUpdateLeafValue(*trans_shadow->lhs_node, 0, 0, *trans_shadow->lhs_debit * diff, *trans_shadow->lhs_credit * diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, 0, 0, *trans_shadow->rhs_debit * diff, *trans_shadow->rhs_credit * diff);
    emit SUpdateLeafValueOne(node_id_, diff, UNIT_COST);

    return true;
}
