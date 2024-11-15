#include "tablemodelhelper.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"
#include "tablemodelutils.h"

TableModelHelper::TableModelHelper(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
    if (node_id >= 1)
        sql_->ReadTransHelper(trans_shadow_list_, node_id);
}

void TableModelHelper::RAppendHelperTrans(const TransShadow* trans_shadow)
{
    if (node_id_ != *trans_shadow->helper_node)
        return;

    auto* new_trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
    new_trans_shadow->date_time = trans_shadow->date_time;
    new_trans_shadow->id = trans_shadow->id;
    new_trans_shadow->description = trans_shadow->description;
    new_trans_shadow->code = trans_shadow->code;
    new_trans_shadow->document = trans_shadow->document;
    new_trans_shadow->state = trans_shadow->state;
    new_trans_shadow->unit_price = trans_shadow->unit_price;
    new_trans_shadow->discount_price = trans_shadow->discount_price;
    new_trans_shadow->settled = trans_shadow->settled;
    new_trans_shadow->helper_node = trans_shadow->helper_node;

    new_trans_shadow->lhs_ratio = trans_shadow->lhs_ratio;
    new_trans_shadow->lhs_debit = trans_shadow->lhs_debit;
    new_trans_shadow->lhs_credit = trans_shadow->lhs_credit;
    new_trans_shadow->lhs_node = trans_shadow->lhs_node;

    new_trans_shadow->rhs_node = trans_shadow->rhs_node;
    new_trans_shadow->rhs_ratio = trans_shadow->rhs_ratio;
    new_trans_shadow->rhs_debit = trans_shadow->rhs_debit;
    new_trans_shadow->rhs_credit = trans_shadow->rhs_credit;

    auto row { trans_shadow_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_shadow_list_.emplaceBack(new_trans_shadow);
    endInsertRows();

    // todo 可能需要额外计算
}

void TableModelHelper::RRemoveHelperTrans(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(row));
    endRemoveRows();
}

QVariant TableModelHelper::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumHelper kColumn { index.column() };

    switch (kColumn) {
    case TableEnumHelper::kID:
        return *trans_shadow->id;
    case TableEnumHelper::kDateTime:
        return *trans_shadow->date_time;
    case TableEnumHelper::kCode:
        return *trans_shadow->code;
    case TableEnumHelper::kLhsNode:
        return *trans_shadow->lhs_node;
    case TableEnumHelper::kLhsRatio:
        return *trans_shadow->lhs_ratio;
    case TableEnumHelper::kLhsDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumHelper::kLhsCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumHelper::kDescription:
        return *trans_shadow->description;
    case TableEnumHelper::kUnitPrice:
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
    case TableEnumHelper::kRhsNode:
        return *trans_shadow->rhs_node;
    case TableEnumHelper::kRhsRatio:
        return *trans_shadow->rhs_ratio;
    case TableEnumHelper::kRhsDebit:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TableEnumHelper::kRhsCredit:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TableEnumHelper::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TableEnumHelper::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : QString::number(trans_shadow->document->size());
    default:
        return QVariant();
    }
}

bool TableModelHelper::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumHelper kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };

    switch (kColumn) {
    case TableEnumHelper::kDateTime:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), DATE_TIME, &TransShadow::date_time);
        break;
    case TableEnumHelper::kCode:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), CODE, &TransShadow::code);
        break;
    case TableEnumHelper::kState:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toBool(), STATE, &TransShadow::state);
        break;
    case TableEnumHelper::kDescription:
        TableModelUtils::UpdateField(
            sql_, trans_shadow, info_.transaction, value.toString(), DESCRIPTION, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    default:
        return false;
    }

    return true;
}

void TableModelHelper::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_trans_header.size() - 1)
        return;

    auto Compare = [column, order](const TransShadow* lhs, const TransShadow* rhs) -> bool {
        const TableEnumHelper kColumn { column };

        switch (kColumn) {
        case TableEnumHelper::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnumHelper::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumHelper::kLhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_node < *rhs->lhs_node) : (*lhs->lhs_node > *rhs->lhs_node);
        case TableEnumHelper::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TableEnumHelper::kLhsDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumHelper::kLhsCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TableEnumHelper::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnumHelper::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumHelper::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_ratio < *rhs->rhs_ratio) : (*lhs->rhs_ratio > *rhs->rhs_ratio);
        case TableEnumHelper::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TableEnumHelper::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TableEnumHelper::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnumHelper::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnumHelper::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TableModelHelper::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumHelper kColumn { index.column() };

    switch (kColumn) {
    case TableEnumHelper::kCode:
    case TableEnumHelper::kDescription:
    case TableEnumHelper::kDateTime:
    case TableEnumHelper::kDocument:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        flags &= ~Qt::ItemIsEditable;
        break;
    }

    return flags;
}

QVariant TableModelHelper::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.helper_header.at(section);

    return QVariant();
}

int TableModelHelper::columnCount(const QModelIndex& /*parent*/) const { return info_.helper_header.size(); }
