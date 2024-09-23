#include "treemodelorder.h"

#include <QMimeData>
#include <QQueue>
#include <QRegularExpression>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"

TreeModelOrder::TreeModelOrder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : AbstractTreeModel { sql, info, base_unit, table_hash, separator, parent }
{
    TreeModelOrder::ConstructTree();
}

void TreeModelOrder::RecalculateAncestor(
    Node* node, int first_diff, double second_diff, double discount_diff, double initial_total_diff, double final_total_diff)
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return;

    const int unit { node->unit };
    const int column_start { std::to_underlying(TreeEnumOrder::kFirst) };
    const int column_end { std::to_underlying(TreeEnumOrder::kFinalTotal) };

    QModelIndexList ancestor {};

    for (node = node->parent; node && node != root_; node = node->parent) {
        if (node->unit != unit)
            continue;

        node->first += first_diff;
        node->second += second_diff;
        node->discount += discount_diff;
        node->initial_total += initial_total_diff;
        node->final_total += final_total_diff;

        ancestor.emplaceBack(GetIndex(node->id));
    }

    if (!ancestor.isEmpty())
        emit dataChanged(index(ancestor.first().row(), column_start), index(ancestor.last().row(), column_end), { Qt::DisplayRole });
}

void TreeModelOrder::RecalculateAncestor(Node* node, int old_unit, double old_final_total)
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return;

    const int new_unit { node->unit };
    const int column_start { std::to_underlying(TreeEnumOrder::kFirst) };
    const int column_end { std::to_underlying(TreeEnumOrder::kFinalTotal) };

    const int first { node->first };
    const double second { node->second };
    const double discount { node->discount };
    const double initial_total { node->initial_total };
    const double final_total { node->final_total };

    QModelIndexList ancestor {};

    for (node = node->parent; node && node != root_; node = node->parent) {
        if (node->unit != old_unit && node->unit != new_unit)
            continue;

        if (node->unit == new_unit) {
            node->first += first;
            node->second += second;
            node->discount += discount;
            node->initial_total += initial_total;
            node->final_total += final_total;
        }

        if (node->unit == old_unit) {
            node->first -= first;
            node->second -= second;
            node->discount -= discount;
            node->initial_total -= initial_total;
            node->final_total -= old_final_total;
        }

        ancestor.emplaceBack(GetIndex(node->id));
    }

    if (!ancestor.isEmpty())
        emit dataChanged(index(ancestor.first().row(), column_start), index(ancestor.last().row(), column_end), { Qt::DisplayRole });
}

void TreeModelOrder::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { node_hash_.value(tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateName(node, tmp_node->name);
    UpdateParty(node, tmp_node->party);
    UpdateDiscount(node, tmp_node->discount, tmp_node->initial_total);
    UpdateUnit(node, tmp_node->unit);
    UpdateDescription(node, tmp_node->description);

    // update code, description, note, date_time, node_rule, first, employee, second
    *node = *tmp_node;
    sql_->UpdateNodeSimple(node);
    sql_->UpdateField(info_.node, node->initial_total, INITIAL_TOTAL, node->id);
    sql_->UpdateField(info_.node, node->final_total, FINAL_TOTAL, node->id);
}

void TreeModelOrder::UpdateNodeLocked(const Node* tmp_node)
{
    auto node { node_hash_.value(tmp_node->id) };
    UpdateLocked(node, tmp_node->locked);
}

void TreeModelOrder::ConstructTree()
{
    sql_->BuildTree(node_hash_);

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (const auto& node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    for (auto& node : const_node_hash)
        if (!node->branch)
            RecalculateAncestor(node, node->first, node->second, node->discount, node->initial_total, node->final_total);

    node_hash_.insert(-1, root_);
}

bool TreeModelOrder::UpdateParty(Node* node, int value) { return UpdateField(node, value, PARTY, &Node::party); }

bool TreeModelOrder::UpdateEmployee(Node* node, int value) { return UpdateField(node, value, EMPLOYEE, &Node::employee); }

bool TreeModelOrder::UpdateLocked(Node* node, bool value)
{
    if (node->locked == value)
        return false;

    node->locked = value;
    sql_->UpdateField(info_.node, value, LOCKED, node->id);

    return true;
}

bool TreeModelOrder::UpdateDateTime(Node* node, CString& value) { return UpdateField(node, value, DATE_TIME, &Node::date_time); }

bool TreeModelOrder::UpdateNodeRule(Node* node, bool value)
{
    if (node->node_rule == value || node->branch)
        return false;

    node->node_rule = value;
    sql_->UpdateField(info_.node, value, NODE_RULE, node->id);

    const int coefficient = -2;
    RecalculateAncestor(node, node->first * coefficient, node->second * coefficient, node->discount * coefficient, node->initial_total * coefficient,
        node->final_total * coefficient);

    node->first *= -1;
    node->second *= -1;
    node->discount *= -1;
    node->initial_total *= -1;
    node->final_total *= -1;

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFinalTotal)));

    sql_->UpdateField(info_.node, node->first, FIRST, node->id);
    sql_->UpdateField(info_.node, node->second, SECOND, node->id);
    sql_->UpdateField(info_.node, node->discount, DISCOUNT, node->id);
    sql_->UpdateField(info_.node, node->initial_total, INITIAL_TOTAL, node->id);
    sql_->UpdateField(info_.node, node->final_total, FINAL_TOTAL, node->id);

    return true;
}

bool TreeModelOrder::UpdateUnit(Node* node, int value)
{
    // Cash = 0, Monthly = 1, Pending = 2

    if (node->unit == value || node->branch)
        return false;

    double old_final_total { node->final_total };
    int old_unit { node->unit };

    node->unit = value;

    switch (value) {
    case UNIT_CASH:
    case UNIT_PENDING:
        node->final_total = node->initial_total + node->discount;
        break;
    case UNIT_MONTHLY:
        node->final_total = 0.0;
        break;
    default:
        return false;
    }

    RecalculateAncestor(node, old_unit, old_final_total);

    sql_->UpdateField(info_.node, value, UNIT, node->id);
    sql_->UpdateField(info_.node, value, FINAL_TOTAL, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kFinalTotal));
    return true;
}

bool TreeModelOrder::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));
    return true;
}

bool TreeModelOrder::UpdateDiscount(Node* node, double value_discount, double value_initial_total)
{
    if (node->discount == value_discount)
        return false;

    node->initial_total = value_initial_total;
    node->discount = value_discount;

    sql_->UpdateField(info_.node, value_discount, DISCOUNT, node->id);
    sql_->UpdateField(info_.node, value_discount, INITIAL_TOTAL, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kInitialTotal));
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
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TreeEnumOrder::kLocked:
            return (order == Qt::AscendingOrder) ? (lhs->locked < rhs->locked) : (lhs->locked > rhs->locked);
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

    RecalculateAncestor(node, node->first, node->second, node->discount, node->initial_total, node->final_total);

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

    if (branch)
        sql_->RemoveNode(node_id, true);

    if (!branch) {
        RecalculateAncestor(node, -node->first, -node->second, -node->discount, -node->final_total, -node->initial_total);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

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
    const bool branch { node->branch };

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
        return branch ? -1 : node->node_rule;
    case TreeEnumOrder::kBranch:
        return node->branch;
    case TreeEnumOrder::kUnit:
        return node->unit;
    case TreeEnumOrder::kParty:
        return branch || node->party == 0 ? QVariant() : node->party;
    case TreeEnumOrder::kEmployee:
        return branch || node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumOrder::kDateTime:
        return branch ? QVariant() : node->date_time;
    case TreeEnumOrder::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumOrder::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumOrder::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumOrder::kLocked:
        return node->locked;
    case TreeEnumOrder::kInitialTotal:
        return node->initial_total;
    case TreeEnumOrder::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumOrder kColumn { index.column() };
    const bool unlocked { !node->locked };
    const bool editable { !node->branch && unlocked };

    bool special_case { false };

    switch (kColumn) {
    case TreeEnumOrder::kCode:
        UpdateCode(node, value.toString());
        break;
    case TreeEnumOrder::kDescription:
        if (unlocked)
            special_case = UpdateDescription(node, value.toString());
        break;
    case TreeEnumOrder::kNote:
        UpdateNote(node, value.toString());
        break;
    case TreeEnumOrder::kNodeRule:
        if (editable)
            UpdateNodeRule(node, value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        if (editable)
            UpdateUnit(node, value.toInt());
        break;
    case TreeEnumOrder::kParty:
        if (editable)
            UpdateParty(node, value.toInt());
        break;
    case TreeEnumOrder::kEmployee:
        if (editable)
            UpdateEmployee(node, value.toInt());
        break;
    case TreeEnumOrder::kDateTime:
        if (editable)
            UpdateDateTime(node, value.toString());
        break;
    case TreeEnumOrder::kLocked:
        special_case = UpdateLocked(node, value.toBool());
        break;
    default:
        return false;
    }

    if (editable || special_case)
        emit SUpdateOrder(value, kColumn);

    emit SResizeColumnToContents(index.column());
    return true;
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
    case TreeEnumOrder::kBranch:
    case TreeEnumOrder::kLocked:
    case TreeEnumOrder::kFirst:
    case TreeEnumOrder::kSecond:
    case TreeEnumOrder::kDiscount:
    case TreeEnumOrder::kInitialTotal:
    case TreeEnumOrder::kFinalTotal:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
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
        node->unit = destination_parent->unit; // todo a lot
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