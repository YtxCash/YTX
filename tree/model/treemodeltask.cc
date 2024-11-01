#include "treemodeltask.h"

#include "global/resourcepool.h"

TreeModelTask::TreeModelTask(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(parent)
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    TreeModelHelper::InitializeRoot(root_, default_unit);
    ConstructTreeFPTS();
}

TreeModelTask::~TreeModelTask() { qDeleteAll(node_hash_); }

void TreeModelTask::RUpdateLeafValueTO(int node_id, double diff, CString& node_field)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->branch || diff == 0.0)
        return;

    node->first += diff;

    sql_->UpdateField(info_.node, node->first, node_field, node_id);
}

void TreeModelTask::RUpdateLeafValueFPTO(
    int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double /*settled_diff*/)
{
    auto* node { TreeModelHelper::GetNodeByID(node_hash_, node_id) };
    if (!node || node == root_ || node->branch)
        return;

    if (initial_credit_diff == 0 && initial_debit_diff == 0 && final_debit_diff == 0 && final_credit_diff == 0)
        return;

    bool rule { node->rule };

    double initial_diff { (rule ? 1 : -1) * (initial_credit_diff - initial_debit_diff) };
    double final_diff { (rule ? 1 : -1) * (final_credit_diff - final_debit_diff) };

    node->initial_total += initial_diff;
    node->final_total += final_diff;

    sql_->UpdateNodeValue(node);
    TreeModelHelper::UpdateAncestorValueFPT(mutex_, root_, node, initial_diff, final_diff);

    emit SUpdateDSpinBox();
}

void TreeModelTask::RUpdateMultiLeafTotalFPT(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_diff {};
    double initial_diff {};
    Node* node {};

    for (int node_id : node_list) {
        node = TreeModelHelper::GetNodeByID(node_hash_, node_id);

        if (!node || node->branch)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->LeafTotal(node);
        sql_->UpdateNodeValue(node);

        final_diff = node->final_total - old_final_total;
        initial_diff = node->initial_total - old_initial_total;

        TreeModelHelper::UpdateAncestorValueFPT(mutex_, root_, node, initial_diff, final_diff);
    }

    emit SUpdateDSpinBox();
}

QVariant TreeModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
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
        return node->branch ? node->branch : QVariant();
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

    auto* node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumTask::kCode:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumTask::kDescription:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumTask::kNote:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumTask::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumTask::kBranch:
        UpdateBranchFPTS(node, value.toBool());
        break;
    case TreeEnumTask::kColor:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), COLOR, &Node::date_time);
        break;
    case TreeEnumTask::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumTask::kUnitCost:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toDouble(), UNIT_COST, &Node::first);
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
    TreeModelHelper::SortIterative(root_, Compare);
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

bool TreeModelTask::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto* destination_parent { GetNodeByIndex(parent) };
    if (!destination_parent->branch)
        return false;

    int node_id {};

    if (auto mime { data->data(NODE_ID) }; !mime.isEmpty())
        node_id = QVariant(mime).toInt();

    auto* node { TreeModelHelper::GetNodeByID(node_hash_, node_id) };
    if (!node || node->parent == destination_parent || TreeModelHelper::IsDescendant(destination_parent, node))
        return false;

    auto begin_row { row == -1 ? destination_parent->children.size() : row };
    auto source_row { node->parent->children.indexOf(node) };
    auto source_index { createIndex(node->parent->children.indexOf(node), 0, node) };

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);
        TreeModelHelper::UpdateAncestorValueFPT(mutex_, root_, node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        TreeModelHelper::UpdateAncestorValueFPT(mutex_, root_, node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    TreeModelHelper::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateName(node);
    emit SUpdateComboModel();

    return true;
}

void TreeModelTask::UpdateNodeFPTS(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto* node { TreeModelHelper::GetNodeByID(node_hash_, tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateRuleFPTO(node, tmp_node->rule);
    UpdateUnit(node, tmp_node->unit);
    UpdateBranchFPTS(node, tmp_node->branch);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node);
        emit SUpdateComboModel();
    }

    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->description, DESCRIPTION, &Node::description);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->code, CODE, &Node::code);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->note, NOTE, &Node::note);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->date_time, COLOR, &Node::date_time);
}

Node* TreeModelTask::GetNodeByIndex(const QModelIndex& index) const { return TreeModelHelper::GetNodeByIndex(root_, index); }

bool TreeModelTask::UpdateBranchFPTS(Node* node, bool value)
{
    if (node->branch == value)
        return false;

    const int node_id { node->id };
    const QString path { GetPath(node_id) };
    QString message {};

    message = tr("Cannot change %1 branch,").arg(path);
    if (TreeModelHelper::HasChildrenFPTS(node, message))
        return false;

    message = tr("Cannot change %1 branch,").arg(path);
    if (TreeModelHelper::IsOpenedFPTS(table_hash_, node_id, message))
        return false;

    message = tr("Cannot change %1 branch,").arg(path);
    if (IsReferencedFPTS(node_id, message))
        return false;

    node->branch = value;
    sql_->UpdateField(info_.node, value, BRANCH, node_id);

    (node->branch) ? branch_path_.insert(node_id, leaf_path_.take(node_id)) : leaf_path_.insert(node_id, branch_path_.take(node_id));
    return true;
}

void TreeModelTask::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    TreeModelHelper::UpdateSeparatorFPTS(leaf_path_, branch_path_, old_separator, new_separator);
}

void TreeModelTask::CopyNodeFPTS(Node* tmp_node, int node_id) const { TreeModelHelper::CopyNodeFPTS(node_hash_, tmp_node, node_id); }

void TreeModelTask::SetParent(Node* node, int parent_id) const { TreeModelHelper::SetParent(node_hash_, node, parent_id); }

QStringList TreeModelTask::ChildrenNameFPTS(int node_id, int exclude_child) const
{
    return TreeModelHelper::ChildrenNameFPTS(node_hash_, node_id, exclude_child);
}

QString TreeModelTask::GetPath(int node_id) const { return TreeModelHelper::GetPathFPTS(leaf_path_, branch_path_, node_id); }

void TreeModelTask::LeafPathBranchPathFPT(QStandardItemModel* combo_model) const
{
    TreeModelHelper::LeafPathBranchPathFPT(leaf_path_, branch_path_, combo_model);
}

void TreeModelTask::LeafPathExcludeIDFPTS(QStandardItemModel* combo_model, int exclude_id) const
{
    TreeModelHelper::LeafPathExcludeIDFPTS(leaf_path_, combo_model, exclude_id);
}

QModelIndex TreeModelTask::GetIndex(int node_id) const
{
    if (node_id == -1)
        return QModelIndex();

    auto it = node_hash_.constFind(node_id);
    if (it == node_hash_.constEnd() || !it.value())
        return QModelIndex();

    const Node* node { it.value() };

    if (!node->parent)
        return QModelIndex();

    auto row { node->parent->children.indexOf(node) };
    if (row == -1)
        return QModelIndex();

    return createIndex(row, 0, node);
}

bool TreeModelTask::ChildrenEmpty(int node_id) const { return TreeModelHelper::ChildrenEmpty(node_hash_, node_id); }

void TreeModelTask::SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const
{
    TreeModelHelper::SearchNode(node_hash_, node_list, node_id_list);
}

bool TreeModelTask::RemoveNode(int row, const QModelIndex& parent)
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
        TreeModelHelper::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
        branch_path_.remove(node_id);
        sql_->RemoveNode(node_id, true);
        emit SUpdateName(node);
    }

    if (!branch) {
        TreeModelHelper::UpdateAncestorValueFPT(mutex_, root_, node, -node->initial_total, -node->final_total);

        leaf_path_.remove(node_id);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateComboModel();

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

bool TreeModelTask::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { TreeModelHelper::ConstructPathFPTS(root_, node, separator_) };
    (node->branch ? branch_path_ : leaf_path_).insert(node->id, path);

    emit SSearch();
    emit SUpdateComboModel();
    return true;
}

bool TreeModelTask::IsReferencedFPTS(int node_id, CString& message) const
{
    if (sql_->InternalReference(node_id)) {
        TreeModelHelper::ShowTemporaryTooltipFPTS(tr("%1 it is internal referenced.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelTask::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    TreeModelHelper::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}

bool TreeModelTask::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    const QString path { GetPath(node_id) };
    QString message {};

    message = tr("Cannot change %1 unit,").arg(path);
    if (TreeModelHelper::HasChildrenFPTS(node, message))
        return false;

    message = tr("Cannot change %1 unit,").arg(path);
    if (IsReferencedFPTS(node_id, message))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

    return true;
}

bool TreeModelTask::UpdateRuleFPTO(Node* node, bool value)
{
    if (node->rule == value)
        return false;

    node->rule = value;
    sql_->UpdateField(info_.node, value, RULE, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    if (!node->branch) {
        emit SRule(node->id, value);
        sql_->UpdateNodeValue(node);
    }

    return true;
}

void TreeModelTask::ConstructTreeFPTS()
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
        path = TreeModelHelper::ConstructPathFPTS(root_, node, separator_);

        if (node->branch) {
            branch_path_.insert(node->id, path);
            continue;
        }

        TreeModelHelper::UpdateAncestorValueFPT(mutex_, root_, node, node->initial_total, node->final_total);

        leaf_path_.insert(node->id, path);
    }

    node_hash_.insert(-1, root_);
}
