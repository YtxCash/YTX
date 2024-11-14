#include "tablemodelhelper.h"

#include "component/enumclass.h"

TableModelHelper::TableModelHelper(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
    if (node_id >= 1)
        sql_->ReadTransHelper(trans_shadow_list_, node_id);
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

bool TableModelHelper::setData(const QModelIndex& index, const QVariant& value, int role) {}

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
    case TableEnumHelper::kID:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
