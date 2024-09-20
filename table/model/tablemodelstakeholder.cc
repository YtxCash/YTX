#include "tablemodelstakeholder.h"

#include "component/constvalue.h"
#include "global/resourcepool.h"

TableModelStakeholder::TableModelStakeholder(
    SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, int mark, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
    , mark_ { mark }
{
}

bool TableModelStakeholder::RemoveTrans(int row, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans_shadow { trans_shadow_list_.at(row) };
    int related_node_id { *trans_shadow->related_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (related_node_id != 0)
        sql_->RemoveTrans(*trans_shadow->id);

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelStakeholder::RemoveMulti(const QList<int>& trans_id_list)
{
    int min_row {};
    int trans_id {};

    for (int i = 0; i != trans_shadow_list_.size(); ++i) {
        trans_id = *trans_shadow_list_.at(i)->id;

        if (trans_id_list.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();

            if (min_row == 0)
                min_row = i;

            --i;
        }
    }

    return true;
}

bool TableModelStakeholder::InsertMulti(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->BuildTransShadowList(trans_shadow_list, node_id, trans_id_list);
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
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
        return *trans_shadow->id;
    case TableEnum::kDateTime:
        return *trans_shadow->date_time;
    case TableEnum::kCode:
        return *trans_shadow->code;
    case TableEnum::kRatio:
        return mark_ == 0 ? QVariant() : *trans_shadow->ratio;
    case TableEnum::kDescription:
        return *trans_shadow->description;
    case TableEnum::kDebit:
        return mark_ == 0 ? *trans_shadow->related_debit : QVariant();
    case TableEnum::kRelatedNode:
        return *trans_shadow->related_node == 0 ? QVariant() : *trans_shadow->related_node;
    case TableEnum::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : QString::number(trans_shadow->document->size());
    default:
        return QVariant();
    }
}

bool TableModelStakeholder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto trans_shadow { trans_shadow_list_.at(kRow) };
    int old_related_node { *trans_shadow->related_node };

    bool rel_changed { false };

    switch (kColumn) {
    case TableEnum::kDateTime:
        UpdateDateTime(trans_shadow, value.toString());
        break;
    case TableEnum::kCode:
        UpdateCode(trans_shadow, value.toString());
        break;
    case TableEnum::kDescription:
        UpdateDescription(trans_shadow, value.toString());
        break;
    case TableEnum::kRatio:
        UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TableEnum::kDebit:
        UpdateCommission(trans_shadow, value.toDouble());
        break;
    case TableEnum::kRelatedNode:
        rel_changed = UpdateRelatedNode(trans_shadow, value.toInt());
        break;
    default:
        return false;
    }

    if (old_related_node == 0) {
        if (rel_changed) {
            sql_->InsertTransShadow(trans_shadow);
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (rel_changed)
        sql_->UpdateTrans(*trans_shadow->id);

    emit SResizeColumnToContents(index.column());
    return true;
}

bool TableModelStakeholder::UpdateRatio(TransShadow* trans_shadow, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };

    if (std::abs(*trans_shadow->ratio - value) < tolerance || mark_ == 0)
        return false;

    *trans_shadow->ratio = value;

    if (*trans_shadow->related_node != 0)
        sql_->UpdateField(info_.transaction, value, UNIT_PRICE, *trans_shadow->id);

    return true;
}

bool TableModelStakeholder::UpdateCommission(TransShadow* trans_shadow, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };

    if (std::abs(*trans_shadow->related_debit - value) < tolerance || mark_ != 0)
        return false;

    *trans_shadow->related_debit = value;

    if (*trans_shadow->related_node != 0)
        sql_->UpdateField(info_.transaction, value, COMMISSION, *trans_shadow->id);

    return true;
}

void TableModelStakeholder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
    if (column <= -1 || column >= info_.part_table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnum kColumn { column };

        switch (kColumn) {
        case TableEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnum::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnum::kRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->ratio < *rhs->ratio) : (*lhs->ratio > *rhs->ratio);
        case TableEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnum::kRelatedNode:
            return (order == Qt::AscendingOrder) ? (*lhs->related_node < *rhs->related_node) : (*lhs->related_node > *rhs->related_node);
        case TableEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
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
