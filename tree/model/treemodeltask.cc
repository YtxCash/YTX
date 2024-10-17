#include "treemodeltask.h"

TreeModelTask::TreeModelTask(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    ConstructTree();
}

void TreeModelTask::RUpdateLeafValueOne(int node_id, double diff, CString& node_field)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->branch || diff == 0.0)
        return;

    node->first += diff;

    sql_->UpdateField(info_.node, node->first, node_field, node_id);
}

QVariant TreeModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

    const TreeEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumTask::kName:
        return node->name;
    case TreeEnumTask::kID:
        return node->id;
    case TreeEnumTask::kCode:
        return node->code;
    case TreeEnumTask::kDescription:
        return node->description;
    case TreeEnumTask::kNote:
        return node->note;
    case TreeEnumTask::kRule:
        return node->rule;
    case TreeEnumTask::kBranch:
        return node->branch;
    case TreeEnumTask::kUnit:
        return node->unit;
    case TreeEnumTask::kColor:
        return node->date_time;
    case TreeEnumTask::kUnitCost:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumTask::kQuantity:
        return node->initial_total == 0 ? QVariant() : node->initial_total;
    case TreeEnumTask::kAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelTask::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumTask::kCode:
        UpdateField(node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumTask::kDescription:
        UpdateField(node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumTask::kNote:
        UpdateField(node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumTask::kRule:
        UpdateRule(node, value.toBool());
        break;
    case TreeEnumTask::kBranch:
        UpdateBranch(node, value.toBool());
        break;
    case TreeEnumTask::kColor:
        UpdateField(node, value.toString(), COLOR, &Node::date_time);
        break;
    case TreeEnumTask::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumTask::kUnitCost:
        UpdateField(node, value.toDouble(), UNIT_COST, &Node::first);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TreeModelTask::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumTask kColumn { column };
        switch (kColumn) {
        case TreeEnumTask::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumTask::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumTask::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumTask::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumTask::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumTask::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumTask::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumTask::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumTask::kUnitCost:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumTask::kQuantity:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumTask::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags TreeModelTask::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumTask::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumTask::kQuantity:
    case TreeEnumTask::kAmount:
    case TreeEnumTask::kBranch:
    case TreeEnumTask::kColor:
    case TreeEnumTask::kUnitCost:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

void TreeModelTask::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { const_cast<Node*>(GetNodeByID(tmp_node->id)) };
    if (*node == *tmp_node)
        return;

    UpdateRule(node, tmp_node->rule);
    UpdateUnit(node, tmp_node->unit);
    UpdateBranch(node, tmp_node->branch);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node);
    }

    UpdateField(node, tmp_node->description, DESCRIPTION, &Node::description);
    UpdateField(node, tmp_node->code, CODE, &Node::code);
    UpdateField(node, tmp_node->note, NOTE, &Node::note);
    UpdateField(node, tmp_node->date_time, COLOR, &Node::date_time);
}
