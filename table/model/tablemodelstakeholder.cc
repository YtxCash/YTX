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

    auto trans { trans_list_.at(row) };
    int related_node_id { *trans->related_node };

    beginRemoveRows(parent, row, row);
    trans_list_.removeAt(row);
    endRemoveRows();

    if (related_node_id != 0)
        sql_->RemoveTransaction(*trans->id);

    ResourcePool<Trans>::Instance().Recycle(trans);
    return true;
}

bool TableModelStakeholder::RemoveMulti(const QList<int>& trans_id_list)
{
    int min_row {};
    int trans_id {};

    for (int i = 0; i != trans_list_.size(); ++i) {
        trans_id = *trans_list_.at(i)->id;

        if (trans_id_list.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<Trans>::Instance().Recycle(trans_list_.takeAt(i));
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
    auto row { trans_list_.size() };
    TransList trans_list {};

    sql_->BuildTransList(trans_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_list.size() - 1);
    trans_list_.append(trans_list);
    endInsertRows();

    return true;
}

QVariant TableModelStakeholder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans { trans_list_.at(index.row()) };
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
        return *trans->id;
    case TableEnum::kDateTime:
        return *trans->date_time;
    case TableEnum::kCode:
        return *trans->code;
    case TableEnum::kRatio:
        return mark_ == 0 ? QVariant() : *trans->ratio;
    case TableEnum::kDescription:
        return *trans->description;
    case TableEnum::kDebit:
        return mark_ == 0 ? *trans->related_debit : QVariant();
    case TableEnum::kRelatedNode:
        return *trans->related_node == 0 ? QVariant() : *trans->related_node;
    case TableEnum::kDocument:
        return trans->document->isEmpty() ? QVariant() : QString::number(trans->document->size());
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

    auto trans { trans_list_.at(kRow) };
    int old_related_node { *trans->related_node };

    bool rel_changed { false };

    switch (kColumn) {
    case TableEnum::kDateTime:
        UpdateDateTime(trans, value.toString());
        break;
    case TableEnum::kCode:
        UpdateCode(trans, value.toString());
        break;
    case TableEnum::kDescription:
        UpdateDescription(trans, value.toString());
        break;
    case TableEnum::kRatio:
        UpdateRatio(trans, value.toDouble());
        break;
    case TableEnum::kDebit:
        UpdateCommission(trans, value.toDouble());
        break;
    case TableEnum::kRelatedNode:
        rel_changed = UpdateRelatedNode(trans, value.toInt());
        break;
    default:
        return false;
    }

    if (old_related_node == 0) {
        if (rel_changed) {
            sql_->InsertTrans(trans);
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (rel_changed)
        sql_->UpdateTransaction(*trans->id);

    emit SResizeColumnToContents(index.column());
    return true;
}

bool TableModelStakeholder::UpdateRatio(Trans* trans, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };

    if (std::abs(*trans->ratio - value) < tolerance || mark_ == 0)
        return false;

    *trans->ratio = value;

    if (*trans->related_node != 0)
        sql_->UpdateField(info_.transaction, value, UNIT_PRICE, *trans->id);

    return true;
}

bool TableModelStakeholder::UpdateCommission(Trans* trans, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };

    if (std::abs(*trans->related_debit - value) < tolerance || mark_ != 0)
        return false;

    *trans->related_debit = value;

    if (*trans->related_node != 0)
        sql_->UpdateField(info_.transaction, value, COMMISSION, *trans->id);

    return true;
}

void TableModelStakeholder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
    if (column <= -1 || column >= info_.part_table_header.size() - 1)
        return;

    auto Compare = [column, order](Trans* lhs, Trans* rhs) -> bool {
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
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
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
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}
