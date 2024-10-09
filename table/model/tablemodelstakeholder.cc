#include "tablemodelstakeholder.h"

#include "component/constvalue.h"
#include "global/resourcepool.h"

TableModelStakeholder::TableModelStakeholder(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
}

bool TableModelStakeholder::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0)
        sql_->RemoveTrans(*trans_shadow->id);

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelStakeholder::RemoveMultiTrans(const QSet<int>& trans_id_set)
{
    if (trans_id_set.isEmpty())
        return false;

    int min_row { -1 };
    int trans_id {};

    for (int i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        trans_id = *trans_shadow_list_.at(i)->id;

        if (trans_id_set.contains(trans_id)) {
            if (min_row == -1 || i < min_row)
                min_row = i;

            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

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

QVariant TableModelStakeholder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans_shadow { trans_shadow_list_.at(index.row()) };
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
        return *trans_shadow->state;
    case TableEnumStakeholder::kInsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;

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

    auto trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };

    bool rhs_changed { false };

    switch (kColumn) {
    case TableEnumStakeholder::kDateTime:
        UpdateField(trans_shadow, value.toString(), DATE_TIME, &TransShadow::date_time);
        break;
    case TableEnumStakeholder::kCode:
        UpdateField(trans_shadow, value.toString(), CODE, &TransShadow::code);
        break;
    case TableEnumStakeholder::kUnitPrice:
        UpdateField(trans_shadow, value.toDouble(), UNIT_PRICE, &TransShadow::unit_price);
        break;
    case TableEnumStakeholder::kDescription:
        UpdateField(trans_shadow, value.toString(), DESCRIPTION, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TableEnumStakeholder::kState:
        UpdateField(trans_shadow, value.toBool(), STATE, &TransShadow::state);
        break;
    case TableEnumStakeholder::kInsideProduct:
        rhs_changed = UpdateRhsNode(trans_shadow, value.toInt());
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
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
    case TableEnum::kDocument:
    case TableEnum::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}
