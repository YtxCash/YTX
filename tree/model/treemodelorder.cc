#include "treemodelorder.h"

#include <QMimeData>
#include <QRegularExpression>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"

TreeModelOrder::TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel { sql, info, default_unit, table_hash, separator, parent }
    , sql_ { static_cast<SqliteOrder*>(sql) }
{
}

void TreeModelOrder::RUpdateLeafValueOne(int node_id, double diff, CString& node_field)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->branch || diff == 0.0)
        return;

    node->first += diff;

    sql_->UpdateField(info_.node, node->first, node_field, node_id);

    const int column { std::to_underlying(TreeEnumOrder::kFirst) };

    auto index { GetIndex(node_id) };
    emit dataChanged(index.siblingAtColumn(column), index.siblingAtColumn(column));

    UpdateAncestorValue(node, diff);
}

void TreeModelOrder::RUpdateLeafValue(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->branch)
        return;

    if (first_diff == 0 && second_diff == 0 && amount_diff == 0 && discount_diff == 0 && settled_diff == 0)
        return;

    double settled { node->unit == UNIT_CASH ? settled_diff : 0.0 };

    node->first += first_diff;
    node->second += second_diff;
    node->discount += discount_diff;
    node->initial_total += amount_diff;
    node->final_total += settled;

    sql_->UpdateNodeValue(node);

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kSettled)));

    UpdateAncestorValue(node, first_diff, second_diff, amount_diff, discount_diff, settled);
}

bool TreeModelOrder::RUpdateStakeholderReference(int old_node_id, int new_node_id)
{
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (node->party == old_node_id)
            node->party = new_node_id;

        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }

    return true;
}

void TreeModelOrder::RUpdateLocked(int node_id, bool checked)
{
    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return;

    auto* node { it.value() };

    int coefficient = checked ? 1 : -1;
    UpdateAncestorValue(node, coefficient * node->first, coefficient * node->second, coefficient * node->initial_total, coefficient * node->discount,
        coefficient * node->final_total);
}

void TreeModelOrder::UpdateAncestorValue(Node* node, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    if (!node || node == root_ || node->parent == root_ || !node->parent)
        return;

    if (first_diff == 0 && second_diff == 0 && amount_diff == 0 && discount_diff == 0 && settled_diff == 0)
        return;

    const int unit { node->unit };
    const int column_begin { std::to_underlying(TreeEnumOrder::kFirst) };
    int column_end { std::to_underlying(TreeEnumOrder::kSettled) };

    // 确定需要更新的列范围
    if (second_diff == 0.0 && amount_diff == 0.0 && discount_diff == 0.0 && settled_diff == 0.0)
        column_end = column_begin;

    QModelIndexList ancestor {};
    for (node = node->parent; node && node != root_; node = node->parent) {
        if (node->unit != unit)
            continue;

        node->first += first_diff;
        node->second += second_diff;
        node->discount += discount_diff;
        node->initial_total += amount_diff;
        node->final_total += settled_diff;

        ancestor.emplaceBack(GetIndex(node->id));
    }

    if (!ancestor.isEmpty())
        emit dataChanged(index(ancestor.first().row(), column_begin), index(ancestor.last().row(), column_end), { Qt::DisplayRole });
}

void TreeModelOrder::ConstructTreeOrder(const QDate& start_date, const QDate& end_date)
{
    sql_->ReadNode(node_hash_, start_date, end_date);

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    for (auto* node : const_node_hash)
        if (!node->branch && node->locked)
            UpdateAncestorValue(node, node->first, node->second, node->initial_total, node->discount, node->final_total);

    node_hash_.insert(-1, root_);
}

bool TreeModelOrder::UpdateRule(Node* node, bool value)
{
    if (node->rule == value || node->branch)
        return false;

    node->rule = value;
    sql_->UpdateField(info_.node, value, RULE, node->id);

    node->first *= -1;
    node->second *= -1;
    node->discount *= -1;
    node->initial_total *= -1;
    node->final_total *= -1;

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kSettled)));

    sql_->UpdateField(info_.node, node->first, FIRST, node->id);
    sql_->UpdateField(info_.node, node->second, SECOND, node->id);
    sql_->UpdateField(info_.node, node->discount, DISCOUNT, node->id);
    sql_->UpdateField(info_.node, node->initial_total, AMOUNT, node->id);
    sql_->UpdateField(info_.node, node->final_total, SETTLED, node->id);

    return true;
}

bool TreeModelOrder::UpdateUnit(Node* node, int value)
{
    // Cash = 0, Monthly = 1, Pending = 2

    if (node->unit == value || node->branch)
        return false;

    node->unit = value;

    switch (value) {
    case UNIT_CASH:
        node->final_total = node->initial_total - node->discount;
        break;
    case UNIT_PENDING:
    case UNIT_MONTHLY:
        node->final_total = 0.0;
        break;
    default:
        return false;
    }

    sql_->UpdateField(info_.node, value, UNIT, node->id);
    sql_->UpdateField(info_.node, node->final_total, AMOUNT, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kSettled));
    return true;
}

bool TreeModelOrder::UpdateLocked(Node* node, bool value)
{
    if (node->locked == value)
        return false;

    int coefficient = value ? 1 : -1;

    UpdateAncestorValue(node, coefficient * node->first, coefficient * node->second, coefficient * node->initial_total, coefficient * node->discount,
        coefficient * node->final_total);

    node->locked = value;
    emit SUpdateData(node->id, TreeEnumOrder::kLocked, value);
    sql_->UpdateField(info_.node, value, LOCKED, node->id);
    return true;
}

bool TreeModelOrder::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));
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
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumOrder::kSettled:
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

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    UpdateAncestorValue(node, node->first, node->second, node->initial_total, node->discount, node->final_total);

    emit SSearch();
    return true;
}

bool TreeModelOrder::RemoveNode(int row, const QModelIndex& parent)
{
    if (row <= -1 || row >= rowCount(parent))
        return false;

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    int node_id { node->id };
    bool branch { node->branch };

    beginRemoveRows(parent, row, row);
    if (branch) {
        for (auto* child : node->children) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }
    }
    parent_node->children.removeOne(node);
    endRemoveRows();

    if (branch)
        sql_->RemoveNode(node_id, true);

    if (!branch) {
        UpdateAncestorValue(node, -node->first, -node->second, -node->initial_total, -node->discount, -node->final_total);
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

    auto* node { GetNodeByIndex(index) };
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
    case TreeEnumOrder::kRule:
        return node->branch ? -1 : node->rule;
    case TreeEnumOrder::kBranch:
        return node->branch ? node->branch : QVariant();
    case TreeEnumOrder::kUnit:
        return node->unit;
    case TreeEnumOrder::kParty:
        return node->party == 0 ? QVariant() : node->party;
    case TreeEnumOrder::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumOrder::kDateTime:
        return node->date_time.isEmpty() ? QVariant() : node->date_time;
    case TreeEnumOrder::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumOrder::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumOrder::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumOrder::kLocked:
        return node->locked ? node->locked : QVariant();
    case TreeEnumOrder::kAmount:
        return node->initial_total;
    case TreeEnumOrder::kSettled:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumOrder::kCode:
        UpdateField(node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumOrder::kDescription:
        UpdateField(node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumOrder::kNote:
        UpdateField(node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumOrder::kRule:
        UpdateRule(node, value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumOrder::kParty:
        UpdateField(node, value.toInt(), PARTY, &Node::party);
        break;
    case TreeEnumOrder::kEmployee:
        UpdateField(node, value.toInt(), EMPLOYEE, &Node::employee);
        break;
    case TreeEnumOrder::kDateTime:
        UpdateField(node, value.toString(), DATE_TIME, &Node::date_time);
        break;
    case TreeEnumOrder::kLocked:
        UpdateLocked(node, value.toBool());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    emit SUpdateData(node->id, kColumn, value);
    return true;
}

Qt::ItemFlags TreeModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumOrder kColumn { index.column() };

    const bool locked { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kLocked)).data().toBool() };
    if (!locked)
        flags |= Qt::ItemIsEditable;

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
    case TreeEnumOrder::kAmount:
    case TreeEnumOrder::kSettled:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        break;
    }

    return flags;
}

bool TreeModelOrder::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto* destination_parent { GetNodeByIndex(parent) };
    if (!destination_parent->branch)
        return false;

    int node_id {};

    if (auto mime { data->data(NODE_ID) }; !mime.isEmpty())
        node_id = QVariant(mime).toInt();

    auto* node { GetNodeByID(node_id) };
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
