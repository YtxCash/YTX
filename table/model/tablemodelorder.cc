#include "tablemodelorder.h"

#include "global/resourcepool.h"

TableModelOrder::TableModelOrder(SPSqlite sql, bool rule, int node_id, CInfo& info, TreeModel* product, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
    , product_ { product }
{
}

void TableModelOrder::RUpdateNodeID(int node_id)
{
    for (auto* trans_shadow : trans_shadow_list_)
        if (*trans_shadow->lhs_node != 0)
            *trans_shadow->node_id = node_id;

    // 一次向数据库添加多条交易
    sql_->WriteTransRange(trans_shadow_list_);
    node_id_ = node_id;
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
    case TableEnumOrder::kInsideProduct:
        return *trans_shadow->lhs_node == 0 ? QVariant() : *trans_shadow->lhs_node;
    case TableEnumOrder::kUnitPrice:
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
    case TableEnumOrder::kSecond:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumOrder::kDescription:
        return *trans_shadow->description;
    case TableEnumOrder::kColor:
        return *trans_shadow->lhs_node == 0 ? QVariant() : product_->Color(*trans_shadow->lhs_node);
    case TableEnumOrder::kNodeID:
        return *trans_shadow->node_id;
    case TableEnumOrder::kFirst:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumOrder::kInitialSubtotal:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TableEnumOrder::kDiscount:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TableEnumOrder::kDiscountPrice:
        return *trans_shadow->discount_price == 0 ? QVariant() : *trans_shadow->discount_price;
    case TableEnumOrder::kOutsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
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
        UpdateField(trans_shadow, value.toString(), CODE, &TransShadow::description);
        break;
    case TableEnumOrder::kDescription:
        UpdateField(trans_shadow, value.toString(), DESCRIPTION, &TransShadow::description);
        break;
    case TableEnumOrder::kInsideProduct:
        UpdateInsideProduct(trans_shadow, value.toInt());
        break;
    case TableEnumOrder::kUnitPrice:
        UpdateUnitPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kSecond:
        UpdateSecond(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kFirst:
        UpdateField(trans_shadow, value.toDouble(), FIRST, &TransShadow::lhs_debit);
        break;
    case TableEnumOrder::kDiscountPrice:
        UpdateDiscountPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kOutsideProduct:
        UpdateField(trans_shadow, value.toInt(), OUTSIDE_PRODUCT, &TransShadow::rhs_node);
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
        case TableEnumOrder::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_node < *rhs->lhs_node) : (*lhs->lhs_node > *rhs->lhs_node);
        case TableEnumOrder::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        case TableEnumOrder::kFirst:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumOrder::kSecond:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TableEnumOrder::kInitialSubtotal:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TableEnumOrder::kDiscount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TableEnumOrder::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->discount_price < *rhs->discount_price) : (*lhs->discount_price > *rhs->discount_price);
        case TableEnumOrder::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
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
    case TableEnumOrder::kColor:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TableModelOrder::insertRows(int row, int /*count*/, const QModelIndex& parent)
{
    auto trans_shadow { sql_->AllocateTransShadow() };

    *trans_shadow->node_id = node_id_;

    beginInsertRows(parent, row, row);
    trans_shadow_list_.emplaceBack(trans_shadow);
    endInsertRows();

    return true;
}

bool TableModelOrder::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans_shadow { trans_shadow_list_.at(row) };
    int node_id { *trans_shadow->node_id };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (node_id != 0)
        sql_->RemoveTrans(*trans_shadow->id);

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelOrder::UpdateInsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->lhs_node == value)
        return false;

    *trans_shadow->lhs_node = value;
    UpdateField(trans_shadow, value, INSIDE_PRODUCT, &TransShadow::lhs_node);

    // todo:  更新单价，更新客户相应的产品编号

    return true;
}

bool TableModelOrder::UpdateUnitPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->unit_price - value) < TOLERANCE)
        return false;

    auto diff { *trans_shadow->lhs_credit * (value - *trans_shadow->unit_price) };
    *trans_shadow->rhs_credit += diff;
    *trans_shadow->unit_price = value;

    UpdateField(trans_shadow, value, UNIT_PRICE, &TransShadow::unit_price);
    UpdateField(trans_shadow, *trans_shadow->rhs_credit, INITIAL_SUBTOTAL, &TransShadow::rhs_credit);

    return true;
}

bool TableModelOrder::UpdateDiscountPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->discount_price - value) < TOLERANCE)
        return false;

    auto diff { *trans_shadow->lhs_credit * (value - *trans_shadow->discount_price) };
    *trans_shadow->rhs_debit += diff;
    *trans_shadow->discount_price = value;

    return true;
}

bool TableModelOrder::UpdateSecond(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->lhs_credit - value) < TOLERANCE)
        return false;

    auto diff { value - *trans_shadow->lhs_credit };
    *trans_shadow->rhs_credit += *trans_shadow->discount_price * diff;
    *trans_shadow->rhs_debit += *trans_shadow->discount_price * diff;

    return true;
}
