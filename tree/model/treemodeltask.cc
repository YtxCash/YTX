#include "treemodeltask.h"

#include "global/resourcepool.h"

TreeModelTask::TreeModelTask(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    leaf_model_ = new QStandardItemModel(this);
    ConstructTree();
}

TreeModelTask::~TreeModelTask() { qDeleteAll(node_hash_); }

void TreeModelTask::RUpdateLeafValueOne(int node_id, double diff, CString& node_field)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->branch || diff == 0.0 || node->unit != UNIT_PROD)
        return;

    node->first += (node->rule ? 1 : -1) * diff;

    sql_->UpdateField(info_.node, node->first, node_field, node_id);
}

void TreeModelTask::RUpdateLeafValue(
    int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double /*settled_diff*/)
{
    auto* node { TreeModelUtils::GetNodeByID(node_hash_, node_id) };
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
    TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, initial_diff, final_diff);

    emit SUpdateDSpinBox();
}

void TreeModelTask::RUpdateMultiLeafTotal(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_diff {};
    double initial_diff {};
    Node* node {};

    for (int node_id : node_list) {
        node = TreeModelUtils::GetNodeByID(node_hash_, node_id);

        if (!node || node->branch)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->LeafTotal(node);
        sql_->UpdateNodeValue(node);

        final_diff = node->final_total - old_final_total;
        initial_diff = node->initial_total - old_initial_total;

        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, initial_diff, final_diff);
    }

    emit SUpdateDSpinBox();
}

QVariant TreeModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
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
    case TreeEnumTask::kIsHelper:
        return node->is_helper ? node->is_helper : QVariant();
    case TreeEnumTask::kColor:
        return node->color;
    case TreeEnumTask::kDateTime:
        return node->branch ? QVariant() : node->date_time;
    case TreeEnumTask::kFinished:
        return node->finished ? node->finished : QVariant();
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
    if (node == root_)
        return false;

    const TreeEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumTask::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumTask::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumTask::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumTask::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumTask::kBranch:
        UpdateBranchFPTS(node, value.toBool());
        emit dataChanged(
            index.siblingAtColumn(std::to_underlying(TreeEnumTask::kDateTime)), index.siblingAtColumn(std::to_underlying(TreeEnumTask::kDateTime)));
        break;
    case TreeEnumTask::kColor:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), COLOR, &Node::color);
        break;
    case TreeEnumTask::kDateTime:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), DATE_TIME, &Node::date_time);
        break;
    case TreeEnumTask::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumTask::kUnitCost:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), UNIT_COST, &Node::first);
        break;
    case TreeEnumTask::kFinished:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toBool(), FINISHED, &Node::finished);
        break;
    case TreeEnumTask::kIsHelper:
        UpdateHelperFPTS(node, value.toBool());
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
        case TreeEnumTask::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case TreeEnumTask::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumTask::kIsHelper:
            return (order == Qt::AscendingOrder) ? (lhs->is_helper < rhs->is_helper) : (lhs->is_helper > rhs->is_helper);
        case TreeEnumTask::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case TreeEnumTask::kDateTime:
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
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags TreeModelTask::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumTask kColumn { index.column() };

    const bool finished { index.siblingAtColumn(std::to_underlying(TreeEnumTask::kFinished)).data().toBool() };
    if (!finished)
        flags |= Qt::ItemIsEditable;

    switch (kColumn) {
    case TreeEnumTask::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumTask::kQuantity:
    case TreeEnumTask::kAmount:
    case TreeEnumTask::kBranch:
    case TreeEnumTask::kFinished:
    case TreeEnumTask::kIsHelper:
    case TreeEnumTask::kColor:
    case TreeEnumTask::kUnitCost:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
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

    auto* node { TreeModelUtils::GetNodeByID(node_hash_, node_id) };
    if (!node || node->parent == destination_parent || TreeModelUtils::IsDescendant(destination_parent, node))
        return false;

    auto begin_row { row == -1 ? destination_parent->children.size() : row };
    auto source_row { node->parent->children.indexOf(node) };
    auto source_index { createIndex(node->parent->children.indexOf(node), 0, node) };

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);
        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, helper_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, helper_path_, helper_model_, node);

    emit SUpdateName(node_id, node->name, node->branch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    return true;
}

void TreeModelTask::UpdateNodeFPTS(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto* node { TreeModelUtils::GetNodeByID(node_hash_, tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateRuleFPTO(node, tmp_node->rule);
    UpdateUnit(node, tmp_node->unit);
    UpdateBranchFPTS(node, tmp_node->branch);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node->id, node->name, node->branch);
    }

    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->description, DESCRIPTION, &Node::description);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->code, CODE, &Node::code);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->note, NOTE, &Node::note);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->color, COLOR, &Node::color);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->date_time, DATE_TIME, &Node::date_time);
}

bool TreeModelTask::UpdateHelperFPTS(Node* node, bool value)
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

    if (TreeModelUtils::IsHelperReferencedFPTS(sql_, node_id, message))
        return false;

    node->is_helper = value;
    sql_->UpdateField(info_.node, value, IS_HELPER, node_id);

    if (node->is_helper) {
        CString path { leaf_path_.take(node_id) };
        helper_path_.insert(node_id, path);
        TreeModelUtils::AddItemToModel(helper_model_, path, node->id);
    } else {
        leaf_path_.insert(node_id, helper_path_.take(node_id));
        TreeModelUtils::RemoveItemFromModel(helper_model_, node_id);
    }

    return true;
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
        TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, helper_path_, root_, node, separator_);
        TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, helper_path_, helper_model_, node);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, branch);
    }

    if (!branch) {
        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, -node->initial_total, -node->final_total);
        leaf_path_.remove(node_id);
        TreeModelUtils::RemoveItemFromModel(leaf_model_, node_id);
    }

    if (node->is_helper) {
        helper_path_.remove(node_id);
        TreeModelUtils::RemoveItemFromModel(helper_model_, node_id);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

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

    QString path { TreeModelUtils::ConstructPathFPTS(root_, node, separator_) };
    (node->branch ? branch_path_ : (node->is_helper ? helper_path_ : leaf_path_)).insert(node->id, path);

    if (node->is_helper) {
        TreeModelUtils::AddItemToModel(helper_model_, path, node->id);
    }

    if (!node->branch) {
        TreeModelUtils::AddItemToModel(leaf_model_, path, node->id);
    }

    emit SSearch();
    return true;
}

bool TreeModelTask::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };

    if (TreeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsHelperReferencedFPTS(sql_, node_id, message))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

    return true;
}

void TreeModelTask::ConstructTree()
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

        if (node->is_helper) {
            helper_path_.insert(node->id, path);
            continue;
        }

        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, node->initial_total, node->final_total);
        leaf_path_.insert(node->id, path);
    }

    TreeModelUtils::HelperPathFPTS(helper_path_, helper_model_, 0, Filter::kIncludeAllWithNone);
    TreeModelUtils::LeafPathRhsNodeFPT(leaf_path_, leaf_model_);
}
