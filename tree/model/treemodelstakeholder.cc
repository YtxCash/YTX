#include "treemodelstakeholder.h"

#include <QMimeData>
#include <QQueue>
#include <QRegularExpression>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"

TreeModelStakeholder::TreeModelStakeholder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel { sql, info, base_unit, table_hash, separator, parent }
{
}

void TreeModelStakeholder::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { const_cast<Node*>(GetNodeByID(tmp_node->id)) };
    if (*node == *tmp_node)
        return;

    UpdateBranch(node, tmp_node->branch);
    UpdateCode(node, tmp_node->code);
    UpdateDescription(node, tmp_node->description);
    UpdateNote(node, tmp_node->note);
    UpdateDeadline(node, tmp_node->date_time);
    UpdatePaymentPeriod(node, tmp_node->first);
    UpdateNodeRule(node, tmp_node->node_rule);
    UpdateEmployee(node, tmp_node->employee);
    UpdateTaxRate(node, tmp_node->second);

    if (node->name != tmp_node->name) {
        UpdateName(node, root_, tmp_node->name);
        emit SUpdateName(node);
    }
}

bool TreeModelStakeholder::UpdateTaxRate(Node* node, double value) { return UpdateField(node, value, TAX_RATE, &Node::second); }

bool TreeModelStakeholder::UpdatePaymentPeriod(Node* node, int value) { return UpdateField(node, value, PAYMENT_PERIOD, &Node::first); }

bool TreeModelStakeholder::UpdateDeadline(Node* node, CString& value) { return UpdateField(node, value, DEADLINE, &Node::date_time); }

bool TreeModelStakeholder::UpdateNodeRule(Node* node, bool value) { return UpdateField(node, value, NODE_RULE, &Node::node_rule); }

bool TreeModelStakeholder::UpdateEmployee(Node* node, int value) { return UpdateField(node, value, EMPLOYEE, &Node::employee); }

void TreeModelStakeholder::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumStakeholder kColumn { column };
        switch (kColumn) {
        case TreeEnumStakeholder::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumStakeholder::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumStakeholder::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumStakeholder::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumStakeholder::kNodeRule:
            return (order == Qt::AscendingOrder) ? (lhs->node_rule < rhs->node_rule) : (lhs->node_rule > rhs->node_rule);
        case TreeEnumStakeholder::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumStakeholder::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumStakeholder::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case TreeEnumStakeholder::kDeadline:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumStakeholder::kPaymentPeriod:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumStakeholder::kTaxRate:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    SortIterative(root_, Compare);
    emit layoutChanged();
}

bool TreeModelStakeholder::RemoveNode(int row, const QModelIndex& parent)
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
        UpdatePath(node, root_);
        branch_path_.remove(node_id);
        sql_->RemoveNode(node_id, true);
    }

    if (!branch) {
        leaf_path_.remove(node_id);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

QVariant TreeModelStakeholder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

    const TreeEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumStakeholder::kName:
        return node->name;
    case TreeEnumStakeholder::kID:
        return node->id;
    case TreeEnumStakeholder::kCode:
        return node->code;
    case TreeEnumStakeholder::kDescription:
        return node->description;
    case TreeEnumStakeholder::kNote:
        return node->note;
    case TreeEnumStakeholder::kNodeRule:
        return node->node_rule;
    case TreeEnumStakeholder::kBranch:
        return node->branch;
    case TreeEnumStakeholder::kUnit:
        return node->unit;
    case TreeEnumStakeholder::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumStakeholder::kDeadline:
        return node->date_time;
    case TreeEnumStakeholder::kPaymentPeriod:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumStakeholder::kTaxRate:
        return node->second == 0 ? QVariant() : node->second;
    default:
        return QVariant();
    }
}

bool TreeModelStakeholder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumStakeholder::kCode:
        UpdateCode(node, value.toString());
        break;
    case TreeEnumStakeholder::kDescription:
        UpdateDescription(node, value.toString());
        break;
    case TreeEnumStakeholder::kNote:
        UpdateNote(node, value.toString());
        break;
    case TreeEnumStakeholder::kNodeRule:
        UpdateNodeRule(node, value.toBool());
        break;
    case TreeEnumStakeholder::kBranch:
        UpdateBranch(node, value.toBool());
        break;
    case TreeEnumStakeholder::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumStakeholder::kEmployee:
        UpdateEmployee(node, value.toInt());
        break;
    case TreeEnumStakeholder::kDeadline:
        UpdateDeadline(node, value.toString());
        break;
    case TreeEnumStakeholder::kPaymentPeriod:
        UpdatePaymentPeriod(node, value.toInt());
        break;
    case TreeEnumStakeholder::kTaxRate:
        UpdateTaxRate(node, value.toDouble());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags TreeModelStakeholder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumStakeholder::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModelStakeholder::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

    auto begin_row { row == -1 ? destination_parent->children.size() : row };
    auto source_row { node->parent->children.indexOf(node) };
    auto source_index { createIndex(node->parent->children.indexOf(node), 0, node) };

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    UpdatePath(node, root_);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));

    return true;
}
