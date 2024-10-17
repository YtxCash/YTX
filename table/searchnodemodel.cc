#include "searchnodemodel.h"

#include "component/enumclass.h"

SearchNodeModel::SearchNodeModel(CInfo& info, const TreeModel* tree_model, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , tree_model_ { tree_model }
{
}

QModelIndex SearchNodeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SearchNodeModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SearchNodeModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int SearchNodeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.search_node_header.size();
}

QVariant SearchNodeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { node_list_.at(index.row()) };
    const TreeEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumOrder::kName:
        return node->name;
    case TreeEnumOrder::kID:
        return node->id;
    case TreeEnumOrder::kCode:
        return node->code;
    case TreeEnumOrder::kDescription:
        return node->description;
    case TreeEnumOrder::kNote:
        return node->note;
    case TreeEnumOrder::kRule:
        return node->rule;
    case TreeEnumOrder::kBranch:
        return node->branch;
    case TreeEnumOrder::kUnit:
        return node->unit;
    case TreeEnumOrder::kParty:
        return node->party == 0 ? QVariant() : node->party;
    case TreeEnumOrder::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumOrder::kDateTime:
        return node->date_time;
    case TreeEnumOrder::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumOrder::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumOrder::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumOrder::kLocked:
        return node->locked == 0 ? QVariant() : node->locked;
    case TreeEnumOrder::kAmount:
        return node->initial_total;
    case TreeEnumOrder::kSettled:
        return node->final_total;
    default:
        return QVariant();
    }
}

QVariant SearchNodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_node_header.at(section);

    return QVariant();
}

void SearchNodeModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumOrder kColumn { column };

        switch (kColumn) {
        case TreeEnumOrder::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumOrder::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumOrder::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumOrder::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumOrder::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumOrder::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumOrder::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumOrder::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->party < rhs->party) : (lhs->party > rhs->party);
        case TreeEnumOrder::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case TreeEnumOrder::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumOrder::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumOrder::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumOrder::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TreeEnumOrder::kLocked:
            return (order == Qt::AscendingOrder) ? (lhs->locked < rhs->locked) : (lhs->locked > rhs->locked);
        case TreeEnumOrder::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(node_list_.begin(), node_list_.end(), Compare);
    emit layoutChanged();
}

void SearchNodeModel::Query(const QString& text)
{
    node_list_.clear();

    beginResetModel();
    tree_model_->SearchNode(node_list_, sql_->SearchNodeName(text));
    endResetModel();
}
