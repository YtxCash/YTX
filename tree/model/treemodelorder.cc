#include "treemodelorder.h"

#include <QMimeData>
#include <QQueue>
#include <QRegularExpression>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"

TreeModelOrder::TreeModelOrder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel { sql, info, base_unit, table_hash, separator, parent }
{
}

void TreeModelOrder::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { node_hash_.value(tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateFirst(node, tmp_node->party);
    UpdateEmployee(node, tmp_node->employee);
    UpdateSecond(node, tmp_node->second);
    UpdateDiscount(node, tmp_node->third);
    UpdateRefund(node, tmp_node->fourth);
    UpdateDateTime(node, tmp_node->date_time);
    UpdateDescription(node, tmp_node->description);
    UpdateNodeRule(node, tmp_node->node_rule);
    UpdateBranch(node, tmp_node->branch);
    UpdateUnit(node, tmp_node->unit);
    UpdateParty(node, tmp_node->first);

    if (node->name != tmp_node->name) {
        UpdateName(node, root_, tmp_node->name);
        emit SUpdateName(node);
    }
}

void TreeModelOrder::IniTree(NodeHash& node_hash)
{
    sql_->BuildTree(node_hash);

    for (auto& node : std::as_const(node_hash)) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    for (auto& node : std::as_const(node_hash))
        UpdateBranchTotal(node, node->party, node->employee, node->initial_total, node->final_total);

    node_hash.insert(-1, root_);
}

void TreeModelOrder::UpdateBranchTotal(const Node* node, double primary_diff, double secondary_diff, double initial_diff, double final_diff)
{
    if (!node)
        return;

    while (node != root_) {
        node->parent->initial_total += initial_diff;
        node->parent->party += primary_diff;
        node->parent->second += secondary_diff;
        node->parent->final_total += final_diff;
        node->parent->note = QString::number(node->note.toDouble() + initial_diff - final_diff);
        node = node->parent;
    }
}

bool TreeModelOrder::UpdateParty(Node* node, int value) { return UpdateField(node, value, STAKEHOLDER, &Node::first); }

bool TreeModelOrder::UpdateFirst(Node* node, int value) { return UpdateField(node, value, FIRST_PROPERTY, &Node::party); }

bool TreeModelOrder::UpdateEmployee(Node* node, int value) { return UpdateField(node, value, EMPLOYEE, &Node::employee); }

bool TreeModelOrder::UpdateSecond(Node* node, double value) { return UpdateField(node, value, THIRD_PROPERTY, &Node::second); }

bool TreeModelOrder::UpdateDiscount(Node* node, double value) { return UpdateField(node, value, DISCOUNT, &Node::third); }

bool TreeModelOrder::UpdateRefund(Node* node, bool value) { return UpdateField(node, value, REFUND, &Node::fourth); }

bool TreeModelOrder::UpdateDateTime(Node* node, CString& value) { return UpdateField(node, value, DATE_TIME, &Node::date_time); }

bool TreeModelOrder::UpdateNodeRule(Node* node, bool value)
{
    if (node->node_rule == value)
        return false;

    node->node_rule = value;
    sql_->UpdateField(info_.node, NODE_RULE, value, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    if (!node->branch)
        emit SNodeRule(node->id, value);

    return true;
}

void TreeModelOrder::sort(int column, Qt::SortOrder order)
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
        case TreeEnumOrder::kNodeRule:
            return (order == Qt::AscendingOrder) ? (lhs->node_rule < rhs->node_rule) : (lhs->node_rule > rhs->node_rule);
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
            return (order == Qt::AscendingOrder) ? (lhs->third < rhs->third) : (lhs->third > rhs->third);
        case TreeEnumOrder::kRefund:
            return (order == Qt::AscendingOrder) ? (lhs->fourth < rhs->fourth) : (lhs->fourth > rhs->fourth);
        case TreeEnumOrder::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumOrder::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    SortIterative(root_, Compare);
    emit layoutChanged();
}

bool TreeModelOrder::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->InsertNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    emit SSearch();
    return true;
}

bool TreeModelOrder::RemoveNode(int row, const QModelIndex& parent)
{
    if (row <= -1 || row >= rowCount(parent))
        return false;

    auto parent_node { GetNodeByIndex(parent) };
    auto node { parent_node->children.at(row) };

    int node_id { node->id };
    bool branch { node->branch };

    beginRemoveRows(parent, row, row);
    if (branch) {
        for (auto& child : node->children) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }
    }
    parent_node->children.removeOne(node);
    endRemoveRows();

    if (branch) {
        sql_->RemoveNode(node_id, true);
        emit SUpdateName(node);
    }

    if (!branch) {
        // UpdateBranchTotal(node, -node->final_total, -node->initial_total);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

bool TreeModelOrder::UpdateLeafTotal(const Node* node)
{
    if (!node || node->branch)
        return false;

    auto node_id { node->id };

    sql_->UpdateField(info_.node, FINAL_TOTAL, node->final_total, node_id);
    sql_->UpdateField(info_.node, INITIAL_TOTAL, node->initial_total, node_id);

    return true;
}

QVariant TreeModelOrder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

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
    case TreeEnumOrder::kNodeRule:
        return node->node_rule;
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
        return node->third == 0 ? QVariant() : node->third;
    case TreeEnumOrder::kRefund:
        return node->fourth;
    case TreeEnumOrder::kInitialTotal:
        return node->initial_total;
    case TreeEnumOrder::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

Qt::ItemFlags TreeModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumOrder::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags &= ~Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModelOrder::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto destination_parent { GetNodeByIndex(parent) };
    if (!destination_parent->branch)
        return false;

    int node_id {};

    if (auto mime { data->data(NODE_ID) }; !mime.isEmpty())
        node_id = QVariant(mime).toInt();

    auto node { GetNodeByID(node_id) };
    if (!node || node->parent == destination_parent || IsDescendant(destination_parent, node))
        return false;

    if (node->unit != destination_parent->unit) {
        node->unit = destination_parent->unit;
    }

    auto begin_row { row == -1 ? destination_parent->children.size() : row };
    auto source_row { node->parent->children.indexOf(node) };
    auto source_index { createIndex(node->parent->children.indexOf(node), 0, node) };

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);
        // UpdateOrderBranchTotal(node, -node->first_property, -node->third_property, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        // UpdateOrderBranchTotal(node, node->first_property, node->third_property, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));
    emit SUpdateName(node);

    return true;
}
