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

void TreeModelOrder::UpdateBranchTotalUp(const Node* node, int first, double second, double discount, double initial_total, double final_total)
{
    // Cash = 0, Monthly = 1, Pending = 2
    // ignore unposted node and monthly node
    if (!node || node->unit == UNIT_MONTHLY)
        return;

    const int coefficient { node->node_rule ? -1 : 1 };
    const int unit { node->unit };

    for (; node && node->parent && node != root_; node = node->parent) {
        if (node->parent->unit != unit)
            continue;

        if (!node->parent->node_rule || (node->parent->node_rule && node->node_rule))
            UpdateNodeValues(node->parent, first, second, discount, initial_total, final_total, node->parent->node_rule ? 1 : coefficient);
    }
}

void TreeModelOrder::UpdateBranchTotalDown(Node* node)
{
    if (!node || !node->branch)
        return;

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};

    double initial_total { 0.0 };
    double final_total { 0.0 };
    double discount_total { 0.0 };
    double second_total { 0.0 };
    int first_total { 0 };

    bool equal {};

    const int unit { node->unit };
    const bool node_rule { node->node_rule };

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        equal = node_rule ? (current->unit == unit && current->node_rule == node_rule) : (current->unit == unit);

        if (current->branch) {
            for (const auto& child : current->children)
                queue.enqueue(child);
        } else if (equal) {
            initial_total += current->initial_total;
            final_total += current->final_total;
            discount_total += current->discount;
            second_total += current->second;
            first_total += current->first;
        }
    }

    node->initial_total = initial_total;
    node->final_total = final_total;
    node->first = first_total;
    node->second = second_total;
    node->discount = discount_total;
}

void TreeModelOrder::UpdateNodeValues(Node* node, int first, double second, double discount, double initial_total, double final_total, int coefficient)
{
    node->first += first * coefficient;
    node->second += second * coefficient;
    node->discount += discount * coefficient;
    node->initial_total += initial_total * coefficient;
    node->final_total += final_total * coefficient;
}

void TreeModelOrder::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { node_hash_.value(tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateParty(node, tmp_node->party);
    UpdateDiscount(node, tmp_node->discount);
    UpdateUnit(node, tmp_node->unit);
    UpdateLocked(node, tmp_node->locked);
    UpdateDescription(node, tmp_node->description);

    // update code, description, note, date_time, node_rule, first, employee, second
    *node = *tmp_node;
    sql_->UpdateNodeSimple(node);
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
        if (node->locked)
            UpdateBranchTotalUp(node, node->first, node->second, node->discount, node->initial_total, node->final_total);

    node_hash_.insert(-1, root_);
}

bool TreeModelOrder::UpdateParty(Node* node, int value) { return UpdateField(node, value, PARTY, &Node::party); }

bool TreeModelOrder::UpdateEmployee(Node* node, int value) { return UpdateField(node, value, EMPLOYEE, &Node::employee); }

bool TreeModelOrder::UpdateDiscount(Node* node, double value)
{
    if (node->discount == value)
        return false;

    node->initial_total -= value - node->discount;
    node->discount = value;

    sql_->UpdateField(info_.node, value, DISCOUNT, node->id);
    sql_->UpdateField(info_.node, value, INITIAL_TOTAL, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kInitialTotal));
    return true;
}

bool TreeModelOrder::UpdateLocked(Node* node, bool value)
{
    if (node->locked == value)
        return false;

    // should set value before UpdateBranchTotal
    node->locked = value;

    const int coefficient = value ? 1 : -1;
    UpdateBranchTotalUp(node, node->first * coefficient, node->second * coefficient, node->discount * coefficient, node->initial_total * coefficient,
        node->final_total * coefficient);

    sql_->UpdateField(info_.node, value, LOCKED, node->id);
    return true;
}

bool TreeModelOrder::UpdateDateTime(Node* node, CString& value) { return UpdateField(node, value, DATE_TIME, &Node::date_time); }

bool TreeModelOrder::UpdateNodeRule(Node* node, bool value)
{
    UpdateField(node, value, NODE_RULE, &Node::node_rule);
    UpdateBranchTotalDown(node);

    return true;
}

bool TreeModelOrder::UpdateUnit(Node* node, int value)
{
    // Cash = 0, Monthly = 1, Pending = 2

    if (node->unit == value)
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node->id);

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

    sql_->UpdateField(info_.node, value, FINAL_TOTAL, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kFinalTotal));
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

    if (node->locked)
        UpdateBranchTotalUp(node, node->first, node->second, node->discount, node->initial_total, node->final_total);

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
        if (node->locked)
            UpdateBranchTotalUp(node, -node->first, -node->second, -node->discount, -node->final_total, -node->initial_total);

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
        return node->node_rule;
    case TreeEnumOrder::kBranch:
        return node->branch;
    case TreeEnumOrder::kUnit:
        return node->unit;
    case TreeEnumOrder::kParty:
        return node->party;
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
        return branch ? QVariant() : node->locked;
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
    if (node->id == -1 || node->branch)
        return false;

    const TreeEnumOrder kColumn { index.column() };
    const bool locked { node->locked };

    switch (kColumn) {
    case TreeEnumOrder::kCode:
        UpdateCode(node, value.toString());
        break;
    case TreeEnumOrder::kDescription:
        UpdateDescription(node, value.toString());
        break;
    case TreeEnumOrder::kNote:
        UpdateNote(node, value.toString());
        break;
    case TreeEnumOrder::kNodeRule:
        if (!locked)
            UpdateNodeRule(node, value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        if (!locked)
            UpdateUnit(node, value.toInt());
        break;
    case TreeEnumOrder::kParty:
        if (!locked)
            UpdateParty(node, value.toInt());
        break;
    case TreeEnumOrder::kEmployee:
        if (!locked)
            UpdateEmployee(node, value.toInt());
        break;
    case TreeEnumOrder::kDateTime:
        if (!locked)
            UpdateDateTime(node, value.toString());
        break;
    case TreeEnumOrder::kDiscount:
        if (!locked)
            UpdateDiscount(node, value.toDouble());
        break;
    case TreeEnumOrder::kLocked:
        if (UpdateLocked(node, value.toBool())) {
            QModelIndexList parents {};

            // 收集所有父节点索引
            auto parent_index { index.parent() };
            while (parent_index.isValid()) {
                parents.append(parent_index);
                parent_index = parent_index.parent();
            }

            if (!parents.isEmpty()) {
                int column_start { std::to_underlying(TreeEnumOrder::kDiscount) };
                int column_end { std::to_underlying(TreeEnumOrder::kInitialTotal) };

                emit dataChanged(this->index(parents.first().row(), column_start), this->index(parents.last().row(), column_end), { Qt::DisplayRole });
            }
        }
        break;
    default:
        return false;
    }

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
    case TreeEnumOrder::kNodeRule:
    case TreeEnumOrder::kLocked:
    case TreeEnumOrder::kFirst:
    case TreeEnumOrder::kSecond:
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
