#include "searchtablemodel.h"

#include "component/enumclass.h"

SearchTableModel::SearchTableModel(CInfo* info, QSharedPointer<SearchSqlite> sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
{
}

SearchTableModel::~SearchTableModel() { trans_list_.clear(); }

QModelIndex SearchTableModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SearchTableModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SearchTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int SearchTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_->table_header.size();
}

QVariant SearchTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans { trans_list_.at(index.row()) };
    const TableEnumSearch kColumn { index.column() };

    switch (kColumn) {
    case TableEnumSearch::kID:
        return trans->id;
    case TableEnumSearch::kDateTime:
        return trans->date_time;
    case TableEnumSearch::kCode:
        return trans->code;
    case TableEnumSearch::kLhsNode:
        return trans->lhs_node;
    case TableEnumSearch::kLhsRatio:
        return trans->lhs_ratio;
    case TableEnumSearch::kLhsDebit:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case TableEnumSearch::kLhsCredit:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case TableEnumSearch::kDescription:
        return trans->description;
    case TableEnumSearch::kRhsNode:
        return trans->rhs_node;
    case TableEnumSearch::kRhsRatio:
        return trans->rhs_ratio;
    case TableEnumSearch::kRhsDebit:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case TableEnumSearch::kRhsCredit:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    case TableEnumSearch::kState:
        return trans->state;
    case TableEnumSearch::kDocument:
        return trans->document.isEmpty() ? QVariant() : QString::number(trans->document.size());
    default:
        return QVariant();
    }
}

QVariant SearchTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_->table_header.at(section);

    return QVariant();
}

void SearchTableModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_->table_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const TableEnumSearch kColumn { column };

        switch (kColumn) {
        case TableEnumSearch::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TableEnumSearch::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TableEnumSearch::kLhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_node < rhs->lhs_node) : (lhs->lhs_node > rhs->lhs_node);
        case TableEnumSearch::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case TableEnumSearch::kLhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case TableEnumSearch::kLhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case TableEnumSearch::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TableEnumSearch::kRhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_node < rhs->rhs_node) : (lhs->rhs_node > rhs->rhs_node);
        case TableEnumSearch::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        case TableEnumSearch::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TableEnumSearch::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case TableEnumSearch::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        case TableEnumSearch::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void SearchTableModel::Query(const QString& text)
{
    if (!trans_list_.isEmpty())
        trans_list_.clear();

    TransList trans_list {};
    if (!text.isEmpty())
        trans_list = sql_->QueryTransList(text);

    beginResetModel();
    for (const auto& trans : trans_list)
        trans_list_.emplace_back(trans);

    endResetModel();
}
