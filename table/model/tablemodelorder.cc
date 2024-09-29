#include "tablemodelorder.h"

TableModelOrder::TableModelOrder(SPSqlite sql, bool rule, int node_id, CInfo& info, TreeModel* product, TreeModel* stakeholder, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
    , product_ { product }
    , stakeholder_ { stakeholder }
{
}

QVariant TableModelOrder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TableEnumOrder::kID:
        return *trans_shadow->id;
    case TableEnumOrder::kCode:
        return *trans_shadow->code;
    case TableEnumOrder::kLhsNode:
        return *trans_shadow->node;
    case TableEnumOrder::kLhsRatio:
        return *trans_shadow->ratio;
    case TableEnumOrder::kSecond:
        return *trans_shadow->credit == 0 ? QVariant() : *trans_shadow->debit;
    case TableEnumOrder::kDescription:
        return product_->Description(*trans_shadow->node);
    case TableEnumOrder::kNodeID:
        return *trans_shadow->node_id == 0 ? QVariant() : *trans_shadow->node_id;
    case TableEnumOrder::kFirst:
        return *trans_shadow->debit == 0 ? QVariant() : *trans_shadow->debit;
    case TableEnumOrder::kInitialSubtotal:
        return *trans_shadow->related_credit;
    case TableEnumOrder::kDiscount:
        return *trans_shadow->related_debit == 0 ? QVariant() : *trans_shadow->related_debit;
    case TableEnumOrder::kRhsRatio:
        return *trans_shadow->related_ratio == 0 ? QVariant() : *trans_shadow->related_ratio;
    case TableEnumOrder::kRhsNode:
        return *trans_shadow->related_node;
    default:
        return QVariant();
    }
}

bool TableModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumOrder kColumn { index.column() };
    const int kRow { index.row() };

    auto trans_shadow { trans_shadow_list_.at(kRow) };

    switch (kColumn) {
    case TableEnumOrder::kCode:
        UpdateCode(trans_shadow, value.toString());
        break;
    case TableEnumOrder::kLhsNode:
        if (UpdateInsideProduct(trans_shadow, value.toInt())) {
            static constexpr auto column { std::to_underlying(TableEnumOrder::kDescription) };
            emit dataChanged(index.siblingAtColumn(column), index.siblingAtColumn(column));
        }
        break;
    case TableEnumOrder::kLhsRatio:
        UpdateUnitPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kSecond:
        UpdateSecond(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kFirst:
        UpdateFirst(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kRhsRatio:
        UpdateDiscountPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kRhsNode:
        *trans_shadow->related_node = value.toInt();
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModelOrder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
    if (column <= -1 || column >= info_.part_table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumOrder kColumn { column };

        switch (kColumn) {
        case TableEnumOrder::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumOrder::kLhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->node < *rhs->node) : (*lhs->node > *rhs->node);
        case TableEnumOrder::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->ratio < *rhs->ratio) : (*lhs->ratio > *rhs->ratio);
        case TableEnumOrder::kFirst:
            return (order == Qt::AscendingOrder) ? (*lhs->debit < *rhs->debit) : (*lhs->debit > *rhs->debit);
        case TableEnumOrder::kSecond:
            return (order == Qt::AscendingOrder) ? (*lhs->credit < *rhs->credit) : (*lhs->credit > *rhs->credit);
        case TableEnumOrder::kInitialSubtotal:
            return (order == Qt::AscendingOrder) ? (*lhs->related_credit < *rhs->related_credit) : (*lhs->related_credit > *rhs->related_credit);
        case TableEnumOrder::kDiscount:
            return (order == Qt::AscendingOrder) ? (*lhs->related_debit < *rhs->related_debit) : (*lhs->related_debit > *rhs->related_debit);
        case TableEnumOrder::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->related_ratio < *rhs->related_ratio) : (*lhs->related_ratio > *rhs->related_ratio);
        case TableEnumOrder::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->related_node < *rhs->related_node) : (*lhs->related_node > *rhs->related_node);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TableModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TableEnumOrder::kID:
    case TableEnumOrder::kNodeID:
    case TableEnumOrder::kInitialSubtotal:
    case TableEnumOrder::kDiscount:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TableModelOrder::UpdateInsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->node == value)
        return false;

    *trans_shadow->node = value;
    return true;
}

bool TableModelOrder::UpdateUnitPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->ratio - value) < TOLERANCE)
        return false;

    auto diff { *trans_shadow->credit * (value - *trans_shadow->ratio) };
    *trans_shadow->related_credit += diff;
    *trans_shadow->ratio = value;

    return true;
}

bool TableModelOrder::UpdateDiscountPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->related_ratio - value) < TOLERANCE)
        return false;

    auto diff { *trans_shadow->credit * (value - *trans_shadow->related_ratio) };
    *trans_shadow->related_debit += diff;
    *trans_shadow->related_ratio = value;

    return true;
}

bool TableModelOrder::UpdateSecond(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->credit - value) < TOLERANCE)
        return false;

    auto diff { value - *trans_shadow->credit };
    *trans_shadow->related_credit += *trans_shadow->ratio * diff;
    *trans_shadow->related_debit += *trans_shadow->related_ratio * diff;

    return true;
}

bool TableModelOrder::UpdateFirst(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->debit - value) < TOLERANCE)
        return false;

    *trans_shadow->debit = value;

    return true;
}

bool TableModelOrder::UpdateCode(TransShadow* trans_shadow, CString& value)
{
    if (*trans_shadow->code == value)
        return false;

    *trans_shadow->code = value;
    return true;
}
