#include "treemodelstakeholder.h"

#include "global/resourcepool.h"

TreeModelStakeholder::TreeModelStakeholder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel { parent }
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    TreeModelHelper::InitializeRoot(root_, default_unit);
    ConstructTree();
}

TreeModelStakeholder::~TreeModelStakeholder()
{
    qDeleteAll(node_hash_);
    delete root_;
}

void TreeModelStakeholder::RUpdateStakeholderSO(int old_node_id, int new_node_id)
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

    auto* node { const_cast<Node*>(TreeModelHelper::GetNodeByID(node_hash_, tmp_node->id)) };
    if (*node == *tmp_node)
        return;

    UpdateBranchFPTS(node, tmp_node->branch);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node->id, node->name, node->branch);
        emit SUpdateComboModel();
    }

    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->description, DESCRIPTION, &Node::description);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->code, CODE, &Node::code);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->note, NOTE, &Node::note);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->first, PAYMENT_PERIOD, &Node::first);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->second, TAX_RATE, &Node::second);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->date_time, DEADLINE, &Node::date_time);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->rule, RULE, &Node::rule);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->employee, EMPLOYEE, &Node::employee);
    TreeModelHelper::UpdateField(sql_, node, info_.node, tmp_node->unit, UNIT, &Node::unit);
}

void TreeModelStakeholder::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    TreeModelHelper::UpdateSeparatorFPTS(leaf_path_, branch_path_, old_separator, new_separator);
    emit SUpdateComboModel();
}

void TreeModelStakeholder::CopyNodeFPTS(Node* tmp_node, int node_id) const { TreeModelHelper::CopyNodeFPTS(node_hash_, tmp_node, node_id); }

void TreeModelStakeholder::SetParent(Node* node, int parent_id) const { TreeModelHelper::SetParent(node_hash_, root_, node, parent_id); }

QStringList TreeModelStakeholder::ChildrenNameFPTS(int node_id, int exclude_child) const
{
    return TreeModelHelper::ChildrenNameFPTS(node_hash_, root_, node_id, exclude_child);
}

QString TreeModelStakeholder::GetPath(int node_id) const { return TreeModelHelper::GetPathFPTS(leaf_path_, branch_path_, node_id); }

void TreeModelStakeholder::LeafPathSpecificUnitPS(QStandardItemModel* combo_model, int unit, UnitFilterMode unit_filter_mode) const
{
    TreeModelHelper::LeafPathSpecificUnitPS(node_hash_, leaf_path_, combo_model, unit, unit_filter_mode);
}

void TreeModelStakeholder::LeafPathSpecificUnitExcludeIDFPTS(QStandardItemModel* combo_model, int unit, int exclude_id) const
{
    TreeModelHelper::LeafPathSpecificUnitExcludeIDFPTS(node_hash_, leaf_path_, combo_model, unit, exclude_id);
}

QModelIndex TreeModelStakeholder::GetIndex(int node_id) const
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

bool TreeModelStakeholder::ChildrenEmpty(int node_id) const { return TreeModelHelper::ChildrenEmpty(node_hash_, node_id); }

void TreeModelStakeholder::SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const
{
    TreeModelHelper::SearchNodeFPTS(node_hash_, node_list, node_id_list);
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

    QString path { TreeModelHelper::ConstructPathFPTS(root_, node, separator_) };
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

QSet<int> TreeModelStakeholder::ChildrenSetFPTS(int node_id) const { return TreeModelHelper::ChildrenSetFPTS(node_hash_, node_id); }

bool TreeModelStakeholder::IsReferencedFPTS(int node_id, CString& message) const
{
    if (sql_->InternalReference(node_id)) {
        TreeModelHelper::ShowTemporaryTooltipFPTS(tr("%1 it is internal referenced.").arg(message), THREE_THOUSAND);
        return true;
    }

    if (sql_->ExternalReference(node_id)) {
        TreeModelHelper::ShowTemporaryTooltipFPTS(tr("%1 it is external referenced.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelStakeholder::UpdateUnit(Node* node, int value)
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

Node* TreeModelStakeholder::GetNodeByIndex(const QModelIndex& index) const { return TreeModelHelper::GetNodeByIndex(root_, index); }

bool TreeModelStakeholder::UpdateBranchFPTS(Node* node, bool value)
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

bool TreeModelStakeholder::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    TreeModelHelper::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
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
        path = TreeModelHelper::ConstructPathFPTS(root_, node, separator_);

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
    TreeModelHelper::SortIterative(root_, Compare);
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
        TreeModelHelper::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
        branch_path_.remove(node_id);
        sql_->RemoveNode(node_id, true);
    }

    if (!branch) {
        leaf_path_.remove(node_id);
        sql_->RemoveNode(node_id, false);
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
    bool skip { node->branch || node->unit == UNIT_PRODUCT || node->rule == RULE_CASH };

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
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnumStakeholder::kDescription:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnumStakeholder::kNote:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnumStakeholder::kRule:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toBool(), RULE, &Node::rule);
        break;
    case TreeEnumStakeholder::kBranch:
        UpdateBranchFPTS(node, value.toBool());
        break;
    case TreeEnumStakeholder::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumStakeholder::kDeadline:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toString(), DEADLINE, &Node::date_time);
        break;
    case TreeEnumStakeholder::kEmployee:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toInt(), EMPLOYEE, &Node::employee);
        break;
    case TreeEnumStakeholder::kPaymentPeriod:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toDouble(), PAYMENT_PERIOD, &Node::first);
        break;
    case TreeEnumStakeholder::kTaxRate:
        TreeModelHelper::UpdateField(sql_, node, info_.node, value.toDouble(), TAX_RATE, &Node::second);
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

    auto* node { TreeModelHelper::GetNodeByID(node_hash_, node_id) };
    if (!node || node->parent == destination_parent || TreeModelHelper::IsDescendant(destination_parent, node))
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
    TreeModelHelper::UpdatePathFPTS(leaf_path_, branch_path_, root_, node, separator_);
    emit SUpdateName(node_id, node->name, node->branch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));
    return true;
}
