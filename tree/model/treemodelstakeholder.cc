#include "treemodelstakeholder.h"

#include "global/resourcepool.h"

TreeModelStakeholder::TreeModelStakeholder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    ConstructTree();
}

TreeModelStakeholder::~TreeModelStakeholder()
{
    qDeleteAll(node_hash_);
    delete root_;
}

void TreeModelStakeholder::RUpdateStakeholder(int old_node_id, int new_node_id)
{
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }
}

void TreeModelStakeholder::UpdateNodeFPTS(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto* node { const_cast<Node*>(TreeModelUtils::GetNodeByID(node_hash_, tmp_node->id)) };
    if (*node == *tmp_node)
        return;

    UpdateBranchFPTS(node, tmp_node->branch);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node->id, node->name, node->branch);
        emit SUpdateComboModel();
    }

    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->description, DESCRIPTION, &Node::description);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->code, CODE, &Node::code);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->note, NOTE, &Node::note);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->first, PAYMENT_PERIOD, &Node::first);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->second, TAX_RATE, &Node::second);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->date_time, DEADLINE, &Node::date_time);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->rule, RULE, &Node::rule);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->employee, EMPLOYEE, &Node::employee);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->unit, UNIT, &Node::unit);
}

bool TreeModelStakeholder::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { TreeModelUtils::ConstructPathFPTS(root_, node, separator_) };
    (node->branch ? branch_path_ : leaf_path_).insert(node->id, path);

    emit SSearch();
    emit SUpdateComboModel();
    return true;
}

QList<int> TreeModelStakeholder::PartyList(CString& text, int unit) const
{
    QList<int> list {};

    for (auto* node : node_hash_)
        if (node->unit == unit && node->name.contains(text))
            list.emplaceBack(node->id);

    return list;
}

bool TreeModelStakeholder::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };

    if (TreeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsExternalReferencedPS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsHelperReferencedFPTS(sql_, node_id, message))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

    return true;
}

bool TreeModelStakeholder::UpdateHelperFPTS(Node* node, bool value)
{
    if (node->is_helper == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 helper,").arg(GetPath(node_id)) };

    if (TreeModelUtils::IsBranchFPTS(node, message))
        return false;

    if (TreeModelUtils::IsOpenedFPTS(table_hash_, node_id, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsExternalReferencedPS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsHelperReferencedFPTS(sql_, node_id, message))
        return false;

    node->is_helper = value;
    sql_->UpdateField(info_.node, value, IS_HELPER, node_id);

    return true;
}

void TreeModelStakeholder::ConstructTree()
{
    sql_->ReadNode(node_hash_);
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    QString path {};
    for (auto* node : const_node_hash) {
        path = TreeModelUtils::ConstructPathFPTS(root_, node, separator_);

        if (node->branch) {
            branch_path_.insert(node->id, path);
            continue;
        }

        leaf_path_.insert(node->id, path);
    }
}

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
        case TreeEnumStakeholder::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumStakeholder::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumStakeholder::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumStakeholder::kIsHelper:
            return (order == Qt::AscendingOrder) ? (lhs->is_helper < rhs->is_helper) : (lhs->is_helper > rhs->is_helper);
        case TreeEnumStakeholder::kDeadline:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumStakeholder::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case TreeEnumStakeholder::kPaymentPeriod:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumStakeholder::kTaxRate:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

bool TreeModelStakeholder::RemoveNode(int row, const QModelIndex& parent)
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

    if (branch) {
        TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
        branch_path_.remove(node_id);
    }

    if (!branch) {
        leaf_path_.remove(node_id);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));
    emit SUpdateComboModel();

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

QVariant TreeModelStakeholder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const TreeEnumStakeholder kColumn { index.column() };
    bool skip { node->branch || node->unit == UNIT_PROD || node->rule == RULE_IM };

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
    case TreeEnumStakeholder::kRule:
        return node->rule;
    case TreeEnumStakeholder::kBranch:
        return node->branch ? node->branch : QVariant();
    case TreeEnumStakeholder::kUnit:
        return node->unit;
    case TreeEnumStakeholder::kIsHelper:
        return node->is_helper ? node->is_helper : QVariant();
    case TreeEnumStakeholder::kDeadline:
        return node->date_time.isEmpty() || skip ? QVariant() : node->date_time;
    case TreeEnumStakeholder::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumStakeholder::kPaymentPeriod:
        return node->first == 0 || skip ? QVariant() : node->first;
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

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const TreeEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumStakeholder::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumStakeholder::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumStakeholder::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumStakeholder::kRule:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toBool(), RULE, &Node::rule);
        break;
    case TreeEnumStakeholder::kBranch:
        UpdateBranchFPTS(node, value.toBool());
        break;
    case TreeEnumStakeholder::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumStakeholder::kDeadline:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), DEADLINE, &Node::date_time);
        break;
    case TreeEnumStakeholder::kEmployee:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), EMPLOYEE, &Node::employee);
        break;
    case TreeEnumStakeholder::kPaymentPeriod:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), PAYMENT_PERIOD, &Node::first);
        break;
    case TreeEnumStakeholder::kTaxRate:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), TAX_RATE, &Node::second);
        break;
    case TreeEnumStakeholder::kIsHelper:
        UpdateHelperFPTS(node, value.toBool());
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
    case TreeEnumStakeholder::kBranch:
    case TreeEnumStakeholder::kIsHelper:
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

    auto* destination_parent { GetNodeByIndex(parent) };
    if (!destination_parent->branch)
        return false;

    int node_id {};

    if (auto mime { data->data(NODE_ID) }; !mime.isEmpty())
        node_id = QVariant(mime).toInt();

    auto* node { TreeModelUtils::GetNodeByID(node_hash_, node_id) };
    if (!node || node->parent == destination_parent || TreeModelUtils::IsDescendant(destination_parent, node))
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
    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SUpdateName(node_id, node->name, node->branch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));
    return true;
}
