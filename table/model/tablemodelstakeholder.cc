#include "tablemodelstakeholder.h"

#include <QDateTime>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "tablemodelhelper.h"

TableModelStakeholder::TableModelStakeholder(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
}

void TableModelStakeholder::RAppendPrice(TransShadow* trans_shadow)
{
    auto row { trans_shadow_list_.size() };
    beginInsertRows(QModelIndex(), row, row);
    trans_shadow_list_.append(trans_shadow);
    endInsertRows();
}

bool TableModelStakeholder::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto* trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0)
        sql_->RemoveTrans(*trans_shadow->id);

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelStakeholder::AppendMultiTrans(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->ReadTransRange(trans_shadow_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();

    return true;
}

bool TableModelStakeholder::UpdateInsideProduct(TransShadow* trans_shadow, int value) const
{
    if (*trans_shadow->rhs_node == value)
        return false;

    *trans_shadow->rhs_node = value;

    return true;
}

QVariant TableModelStakeholder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TableEnumStakeholder::kID:
        return *trans_shadow->id;
    case TableEnumStakeholder::kDateTime:
        return *trans_shadow->date_time;
    case TableEnumStakeholder::kCode:
        return *trans_shadow->code;
    case TableEnumStakeholder::kUnitPrice:
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
    case TableEnumStakeholder::kDescription:
        return *trans_shadow->description;
    case TableEnumStakeholder::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : QString::number(trans_shadow->document->size());
    case TableEnumStakeholder::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TableEnumStakeholder::kInsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnumStakeholder::kOutsideProduct:
        return *trans_shadow->helper_node == 0 ? QVariant() : *trans_shadow->helper_node;
    default:
        return QVariant();
    }
}

bool TableModelStakeholder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumStakeholder kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };

    bool rhs_changed { false };

    switch (kColumn) {
    case TableEnumStakeholder::kDateTime:
        TableModelHelper::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), DATE_TIME, &TransShadow::date_time);
        break;
    case TableEnumStakeholder::kCode:
        TableModelHelper::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), CODE, &TransShadow::code);
        break;
    case TableEnumStakeholder::kInsideProduct:
        rhs_changed = UpdateInsideProduct(trans_shadow, value.toInt());
        break;
    case TableEnumStakeholder::kUnitPrice:
        TableModelHelper::UpdateField(sql_, trans_shadow, info_.transaction, value.toDouble(), UNIT_PRICE, &TransShadow::unit_price);
        break;
    case TableEnumStakeholder::kDescription:
        TableModelHelper::UpdateField(
            sql_, trans_shadow, info_.transaction, value.toString(), DESCRIPTION, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TableEnumStakeholder::kState:
        TableModelHelper::UpdateField(sql_, trans_shadow, info_.transaction, value.toBool(), STATE, &TransShadow::state);
        break;
    case TableEnumStakeholder::kOutsideProduct:
        TableModelHelper::UpdateField(sql_, trans_shadow, info_.transaction, value.toInt(), OUTSIDE_PRODUCT, &TransShadow::helper_node);
        break;
    default:
        return false;
    }

    if (rhs_changed) {
        if (old_rhs_node == 0)
            sql_->WriteTrans(trans_shadow);
        else
            sql_->UpdateField(info_.transaction, value.toInt(), INSIDE_PRODUCT, *trans_shadow->id);
    }

    if (trans_shadow_list_.size() == 1)
        emit SResizeColumnToContents(std::to_underlying(TableEnumStakeholder::kDateTime));

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModelStakeholder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
    if (column <= -1 || column >= info_.table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumStakeholder kColumn { column };

        switch (kColumn) {
        case TableEnumStakeholder::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnumStakeholder::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumStakeholder::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        case TableEnumStakeholder::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnumStakeholder::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnumStakeholder::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        case TableEnumStakeholder::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->helper_node < *rhs->helper_node) : (*lhs->helper_node > *rhs->helper_node);
        case TableEnumStakeholder::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TableModelStakeholder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TableEnumStakeholder::kID:
    case TableEnumStakeholder::kDocument:
    case TableEnumStakeholder::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}
