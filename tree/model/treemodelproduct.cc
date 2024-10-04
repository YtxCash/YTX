#include "treemodelproduct.h"

#include <QMimeData>
#include <QQueue>
#include <QRegularExpression>

#include "component/enumclass.h"

TreeModelProduct::TreeModelProduct(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel { sql, info, base_unit, table_hash, separator, parent }
{
    ConstructTree();
}

void TreeModelProduct::UpdateNode(const Node* tmp_node)
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
    UpdateField(node, tmp_node->second, LHS_RATIO, &Node::discount);
    UpdateField(node, tmp_node->second, COMMISSION, &Node::second);
}

bool TreeModelProduct::UpdateUnitPrice(Node* node, double value, CString& field) { return UpdateField(node, value, field, &Node::discount); }

bool TreeModelProduct::UpdateCommission(Node* node, double value, CString& field) { return UpdateField(node, value, field, &Node::second); }

bool TreeModelProduct::IsReferenced(int node_id, CString& message)
{
    if (sql_->InternalReference(node_id)) {
        ShowTemporaryTooltip(tr("%1 it is internal referenced.").arg(message), 3000);
        return true;
    }

    if (sql_->ExternalReference(node_id)) {
        ShowTemporaryTooltip(tr("%1 it is external referenced.").arg(message), 3000);
        return true;
    }

    return false;
}

bool TreeModelProduct::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    const QString path { GetPath(node_id) };
    QString message {};

    message = tr("Cannot change %1 unit,").arg(path);
    if (HasChildren(node, message))
        return false;

    message = tr("Cannot change %1 unit,").arg(path);
    if (IsReferenced(node_id, message))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

    return true;
}

void TreeModelProduct::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumProduct kColumn { column };
        switch (kColumn) {
        case TreeEnumProduct::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumProduct::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumProduct::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumProduct::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumProduct::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumProduct::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumProduct::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumProduct::kCommission:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumProduct::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TreeEnumProduct::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumProduct::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    SortIterative(root_, Compare);
    emit layoutChanged();
}

QVariant TreeModelProduct::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kName:
        return node->name;
    case TreeEnumProduct::kID:
        return node->id;
    case TreeEnumProduct::kCode:
        return node->code;
    case TreeEnumProduct::kDescription:
        return node->description;
    case TreeEnumProduct::kNote:
        return node->note;
    case TreeEnumProduct::kRule:
        return node->rule;
    case TreeEnumProduct::kBranch:
        return node->branch;
    case TreeEnumProduct::kUnit:
        return node->unit;
    case TreeEnumProduct::kCommission:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumProduct::kUnitPrice:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumProduct::kInitialTotal:
        return node->initial_total;
    case TreeEnumProduct::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelProduct::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kCode:
        UpdateField(node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumProduct::kDescription:
        UpdateField(node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumProduct::kNote:
        UpdateField(node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumProduct::kRule:
        UpdateRule(node, value.toBool());
        break;
    case TreeEnumProduct::kBranch:
        UpdateBranch(node, value.toBool());
        break;
    case TreeEnumProduct::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumProduct::kCommission:
        UpdateCommission(node, value.toDouble());
        break;
    case TreeEnumProduct::kUnitPrice:
        UpdateUnitPrice(node, value.toDouble());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags TreeModelProduct::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumProduct::kInitialTotal:
    case TreeEnumProduct::kFinalTotal:
    case TreeEnumProduct::kBranch:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
