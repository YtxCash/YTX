#include "treemodelfinance.h"

#include "global/resourcepool.h"

TreeModelFinance::TreeModelFinance(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(parent)
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    TreeModelUtils::InitializeRoot(root_, default_unit);
    ConstructTree();
}

TreeModelFinance::~TreeModelFinance()
{
    qDeleteAll(node_hash_);
    delete root_;
}

void TreeModelFinance::RUpdateLeafValueFPTO(
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

void TreeModelFinance::RUpdateMultiLeafTotalFPT(const QList<int>& node_list)
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

bool TreeModelFinance::RemoveNode(int row, const QModelIndex& parent)
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
        emit SUpdateName(node_id, node->name, branch);
    }

    if (!branch) {
        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, -node->initial_total, -node->final_total);
        leaf_path_.remove(node_id);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateComboModel();

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

bool TreeModelFinance::InsertNode(int row, const QModelIndex& parent, Node* node)
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

void TreeModelFinance::UpdateNodeFPTS(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto it { node_hash_.constFind(tmp_node->id) };
    if (it == node_hash_.constEnd())
        return;

    auto* node { it.value() };
    if (*node == *tmp_node)
        return;

    UpdateBranchFPTS(node, tmp_node->branch);
    UpdateRuleFPTO(node, tmp_node->rule);
    UpdateUnit(node, tmp_node->unit);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node->id, node->name, node->branch);
        emit SUpdateComboModel();
    }

    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->description, DESCRIPTION, &Node::description);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->code, CODE, &Node::code);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->note, NOTE, &Node::note);
}

void TreeModelFinance::UpdateDefaultUnit(int default_unit)
{
    if (root_->unit == default_unit)
        return;

    root_->unit = default_unit;

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash)
        if (node->branch && node->unit != default_unit)
            TreeModelUtils::UpdateBranchUnitF(root_, node);
}

QVariant TreeModelFinance::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const TreeEnumFinance kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumFinance::kName:
        return node->name;
    case TreeEnumFinance::kID:
        return node->id;
    case TreeEnumFinance::kCode:
        return node->code;
    case TreeEnumFinance::kDescription:
        return node->description;
    case TreeEnumFinance::kNote:
        return node->note;
    case TreeEnumFinance::kRule:
        return node->rule;
    case TreeEnumFinance::kBranch:
        return node->branch ? node->branch : QVariant();
    case TreeEnumFinance::kUnit:
        return node->unit;
    case TreeEnumFinance::kIsHelper:
        return node->is_helper ? node->is_helper : QVariant();
    case TreeEnumFinance::kInitialTotal:
        return node->unit == root_->unit ? QVariant() : node->initial_total;
    case TreeEnumFinance::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelFinance::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const TreeEnumFinance kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumFinance::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumFinance::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumFinance::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumFinance::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumFinance::kBranch:
        UpdateBranchFPTS(node, value.toBool());
        break;
    case TreeEnumFinance::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumFinance::kIsHelper:
        UpdateHelperFTS(node, value.toBool());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TreeModelFinance::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumFinance kColumn { column };
        switch (kColumn) {
        case TreeEnumFinance::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumFinance::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumFinance::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumFinance::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumFinance::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumFinance::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumFinance::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumFinance::kIsHelper:
            return (order == Qt::AscendingOrder) ? (lhs->is_helper < rhs->is_helper) : (lhs->is_helper > rhs->is_helper);
        case TreeEnumFinance::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumFinance::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags TreeModelFinance::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumFinance kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumFinance::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumFinance::kInitialTotal:
    case TreeEnumFinance::kFinalTotal:
    case TreeEnumFinance::kBranch:
    case TreeEnumFinance::kIsHelper:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModelFinance::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SUpdateName(node_id, node->name, node->branch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateComboModel();

    return true;
}

void TreeModelFinance::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    TreeModelUtils::UpdateSeparatorFPTS(leaf_path_, branch_path_, old_separator, new_separator);
    emit SUpdateComboModel();
}

void TreeModelFinance::CopyNodeFPTS(Node* tmp_node, int node_id) const { TreeModelUtils::CopyNodeFPTS(node_hash_, tmp_node, node_id); }

void TreeModelFinance::SetParent(Node* node, int parent_id) const { TreeModelUtils::SetParent(node_hash_, root_, node, parent_id); }

QStringList TreeModelFinance::ChildrenNameFPTS(int node_id, int exclude_child) const
{
    return TreeModelUtils::ChildrenNameFPTS(node_hash_, root_, node_id, exclude_child);
}

QString TreeModelFinance::GetPath(int node_id) const { return TreeModelUtils::GetPathFPTS(leaf_path_, branch_path_, node_id); }

void TreeModelFinance::PathPreferencesFPT(QStandardItemModel* model) const { TreeModelUtils::PathPreferencesFPT(node_hash_, leaf_path_, branch_path_, model); }

void TreeModelFinance::LeafPathRhsNodeFPT(QStandardItemModel* model, int specific_node, Filter filter) const
{
    TreeModelUtils::LeafPathRhsNodeFPT(node_hash_, leaf_path_, model, specific_node, filter);
}

void TreeModelFinance::LeafPathRemoveNodeFPTS(QStandardItemModel* model, int specific_unit, int exclude_node) const
{
    TreeModelUtils::LeafPathRemoveNodeFPTS(node_hash_, leaf_path_, model, specific_unit, exclude_node);
}

void TreeModelFinance::LeafPathHelperNodeFTS(QStandardItemModel* model, int specific_node, Filter filter) const
{
    TreeModelUtils::LeafPathHelperNodeFTS(node_hash_, leaf_path_, model, specific_node, filter);
}

QModelIndex TreeModelFinance::GetIndex(int node_id) const
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

bool TreeModelFinance::ChildrenEmpty(int node_id) const { return TreeModelUtils::ChildrenEmpty(node_hash_, node_id); }

void TreeModelFinance::SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const
{
    TreeModelUtils::SearchNodeFPTS(node_hash_, node_list, node_id_list);
}

QSet<int> TreeModelFinance::ChildrenSetFPTS(int node_id) const { return TreeModelUtils::ChildrenSetFPTS(node_hash_, node_id); }

Node* TreeModelFinance::GetNodeByIndex(const QModelIndex& index) const { return TreeModelUtils::GetNodeByIndex(root_, index); }

bool TreeModelFinance::UpdateBranchFPTS(Node* node, bool value)
{
    if (node->branch == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 branch,").arg(GetPath(node_id)) };

    if (TreeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (TreeModelUtils::IsOpenedFPTS(table_hash_, node_id, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsHelperFTS(node, message))
        return false;

    node->branch = value;
    sql_->UpdateField(info_.node, value, BRANCH, node_id);

    (node->branch) ? branch_path_.insert(node_id, leaf_path_.take(node_id)) : leaf_path_.insert(node_id, branch_path_.take(node_id));
    return true;
}

bool TreeModelFinance::UpdateHelperFTS(Node* node, bool value)
{
    if (node->is_helper == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 helper,").arg(GetPath(node_id)) };

    if (TreeModelUtils::IsBranchFTS(node, message))
        return false;

    if (TreeModelUtils::IsOpenedFPTS(table_hash_, node_id, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsHelperReferencedFTS(sql_, node_id, message))
        return false;

    node->is_helper = value;
    sql_->UpdateField(info_.node, value, IS_HELPER, node_id);

    return true;
}

void TreeModelFinance::ConstructTree()
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

        TreeModelUtils::UpdateAncestorValueFPT(mutex_, root_, node, node->initial_total, node->final_total);
        leaf_path_.insert(node->id, path);
    }
}

bool TreeModelFinance::UpdateRuleFPTO(Node* node, bool value)
{
    if (node->rule == value)
        return false;

    node->rule = value;
    sql_->UpdateField(info_.node, value, RULE, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    if (!node->branch) {
        emit SRule(info_.section, node->id, value);
        sql_->UpdateNodeValue(node);
    }

    return true;
}

bool TreeModelFinance::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    int node_id { node->id };
    auto message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsHelperReferencedFTS(sql_, node_id, message))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

    if (node->branch)
        TreeModelUtils::UpdateBranchUnitF(root_, node);

    return true;
}

bool TreeModelFinance::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}
