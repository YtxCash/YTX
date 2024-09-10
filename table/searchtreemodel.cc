#include "searchtreemodel.h"

#include "component/enumclass.h"

SearchTreeModel::SearchTreeModel(CInfo& info, const AbstractTreeModel& tree_model, CSectionRule& section_rule, QSharedPointer<SearchSqlite> sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , section_rule_ { section_rule }
    , tree_model_ { tree_model }
{
}

QModelIndex SearchTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SearchTreeModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SearchTreeModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int SearchTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.tree_header.size();
}

QVariant SearchTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { node_list_.at(index.row()) };
    const TreeEnum kColumn { index.column() };

    switch (kColumn) {
    case TreeEnum::kName:
        return node->name;
    case TreeEnum::kID:
        return node->id;
    case TreeEnum::kCode:
        return node->code;
    case TreeEnum::kThird:
        return node->discount;
    case TreeEnum::kDescription:
        return node->description;
    case TreeEnum::kNote:
        return node->note;
    case TreeEnum::kNodeRule:
        return node->node_rule;
    case TreeEnum::kBranch:
        return node->branch;
    case TreeEnum::kUnit:
        return node->unit;
    case TreeEnum::kInitialTotal:
        return node->unit == section_rule_.base_unit ? node->final_total : node->initial_total;
    default:
        return QVariant();
    }
}

QVariant SearchTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.tree_header.at(section);

    return QVariant();
}

void SearchTreeModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnum kColumn { column };

        switch (kColumn) {
        case TreeEnum::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnum::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnum::kThird:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnum::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnum::kNodeRule:
            return (order == Qt::AscendingOrder) ? (lhs->node_rule < rhs->node_rule) : (lhs->node_rule > rhs->node_rule);
        case TreeEnum::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnum::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnum::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(node_list_.begin(), node_list_.end(), Compare);
    emit layoutChanged();
}

void SearchTreeModel::Query(const QString& text)
{
    node_list_.clear();

    beginResetModel();
    tree_model_.NodeList(node_list_, sql_->Node(text));
    endResetModel();
}
