#include "treemodel.h"

#include <QQueue>

#include "component/constvalue.h"
#include "global/resourcepool.h"

TreeModel::TreeModel(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    InitializeRoot(root_, base_unit);
    ConstructTree(root_);
}

TreeModel::~TreeModel() { qDeleteAll(node_hash_); }

bool TreeModel::RUpdateMultiTotal(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_diff {};
    double initial_diff {};
    Node* node {};

    for (const auto& node_id : node_list) {
        node = GetNodeByID(node_id);

        if (!node || node->branch)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->NodeLeafTotal(node);
        UpdateLeafTotal(node, INITIAL_TOTAL, FINAL_TOTAL);

        final_diff = node->final_total - old_final_total;
        initial_diff = node->initial_total - old_initial_total;

        UpdateBranchTotal(node, root_, initial_diff, final_diff);
    }

    emit SUpdateDSpinBox();
    return true;
}

bool TreeModel::RRemoveNode(int node_id)
{
    auto index { GetIndex(node_id) };
    auto row { index.row() };
    auto parent_index { index.parent() };
    RemoveNode(row, parent_index);

    return true;
}

void TreeModel::RUpdateOneTotal(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff)
{
    auto node { GetNodeByID(node_id) };
    auto node_rule { node->node_rule };

    auto initial_diff { node_rule ? initial_credit_diff - initial_debit_diff : initial_debit_diff - initial_credit_diff };
    auto final_diff { node_rule ? final_credit_diff - final_debit_diff : final_debit_diff - final_credit_diff };

    node->initial_total += initial_diff;
    node->final_total += final_diff;

    UpdateLeafTotal(node, INITIAL_TOTAL, FINAL_TOTAL);
    UpdateBranchTotal(node, root_, initial_diff, final_diff);
    emit SUpdateDSpinBox();
}

bool TreeModel::RemoveNode(int row, const QModelIndex& parent)
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
        emit SUpdateName(node);
    }

    if (!branch) {
        UpdateBranchTotal(node, root_, -node->initial_total, -node->final_total);
        leaf_path_.remove(node_id);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

bool TreeModel::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->InsertNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { ConstructPath(node, root_) };
    (node->branch ? branch_path_ : leaf_path_).insert(node->id, path);

    emit SSearch();
    return true;
}

void TreeModel::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { node_hash_.value(tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateBranch(node, tmp_node->branch);
    UpdateCode(node, tmp_node->code, CODE);
    UpdateDescription(node, tmp_node->description, DESCRIPTION);
    UpdateNote(node, tmp_node->note, NOTE);
    UpdateNodeRule(node, tmp_node->node_rule);
    UpdateUnit(node, tmp_node->unit);

    if (node->name != tmp_node->name) {
        UpdateName(node, root_, tmp_node->name);
        emit SUpdateName(node);
    }
}

void TreeModel::UpdateBaseUnit(int base_unit)
{
    if (root_->unit == base_unit)
        return;

    root_->unit = base_unit;

    for (auto node : std::as_const(node_hash_))
        if (node->branch && node->unit != base_unit)
            UpdateBranchUnit(node);
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto parent_node { GetNodeByIndex(parent) };
    auto node { parent_node->children.at(row) };

    return node ? createIndex(row, column, node) : QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{ // root_'s index is QModelIndex();

    if (!index.isValid())
        return QModelIndex();

    auto node { GetNodeByIndex(index) };
    if (node == root_)
        return QModelIndex();

    auto parent_node { node->parent };
    if (parent_node == root_)
        return QModelIndex();

    return createIndex(parent_node->parent->children.indexOf(parent_node), 0, parent_node);
}

int TreeModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

QVariant TreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

    const TreeEnumFinanceTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumFinanceTask::kName:
        return node->name;
    case TreeEnumFinanceTask::kID:
        return node->id;
    case TreeEnumFinanceTask::kCode:
        return node->code;
    case TreeEnumFinanceTask::kDescription:
        return node->description;
    case TreeEnumFinanceTask::kNote:
        return node->note;
    case TreeEnumFinanceTask::kNodeRule:
        return node->node_rule;
    case TreeEnumFinanceTask::kBranch:
        return node->branch;
    case TreeEnumFinanceTask::kUnit:
        return node->unit;
    case TreeEnumFinanceTask::kInitialTotal:
        return node->unit == root_->unit ? QVariant() : node->initial_total;
    case TreeEnumFinanceTask::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumFinanceTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumFinanceTask::kCode:
        UpdateCode(node, value.toString());
        break;
    case TreeEnumFinanceTask::kDescription:
        UpdateDescription(node, value.toString());
        break;
    case TreeEnumFinanceTask::kNote:
        UpdateNote(node, value.toString());
        break;
    case TreeEnumFinanceTask::kNodeRule:
        UpdateNodeRule(node, value.toBool());
        break;
    case TreeEnumFinanceTask::kBranch:
        UpdateBranch(node, value.toBool());
        break;
    case TreeEnumFinanceTask::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TreeModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumFinanceTask kColumn { column };
        switch (kColumn) {
        case TreeEnumFinanceTask::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumFinanceTask::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumFinanceTask::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumFinanceTask::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumFinanceTask::kNodeRule:
            return (order == Qt::AscendingOrder) ? (lhs->node_rule < rhs->node_rule) : (lhs->node_rule > rhs->node_rule);
        case TreeEnumFinanceTask::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumFinanceTask::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumFinanceTask::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumFinanceTask::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumFinanceTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumFinanceTask::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumFinanceTask::kInitialTotal:
    case TreeEnumFinanceTask::kFinalTotal:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
        UpdateBranchTotal(node, root_, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        UpdateBranchTotal(node, root_, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    UpdatePath(node, root_);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateName(node);

    return true;
}

void TreeModel::ComboPathUnit(QComboBox* combo, int unit) const
{
    if (!combo)
        return;

    int id {};
    QString path {};

    for (const auto* node : std::as_const(node_hash_))
        if (node && node->unit == unit) {
            id = node->id;
            path = GetPath(id);
            combo->addItem(path, id);
        }
}

void TreeModel::ComboPathLeaf(QComboBox* combo, int exclude) const
{
    if (!combo)
        return;

    for (const auto& [id, path] : leaf_path_.asKeyValueRange()) {
        if (id != exclude)
            combo->addItem(path, id);
    }
}

void TreeModel::ComboPathLeafBranch(QComboBox* combo) const
{
    if (!combo)
        return;

    for (const auto& [id, path] : leaf_path_.asKeyValueRange())
        combo->addItem(path, id);

    for (const auto& [id, path] : branch_path_.asKeyValueRange())
        combo->addItem(path, id);
}

void TreeModel::ChildrenName(QStringList& list, int node_id, int exclude_child) const
{
    const Node* node { node_hash_.value(node_id, nullptr) };
    if (!node)
        return;

    for (const auto* child : node->children) {
        if (child->id != exclude_child)
            list.emplaceBack(child->name);
    }
}

void TreeModel::SetParent(Node* node, int parent_id) const
{
    if (!node)
        return;

    auto it { node_hash_.constFind(parent_id) };

    node->parent = it == node_hash_.constEnd() ? nullptr : it.value();
}

void TreeModel::CopyNode(Node* tmp_node, int node_id) const
{
    if (!tmp_node)
        return;

    auto it = node_hash_.constFind(node_id);
    if (it == node_hash_.constEnd() || !it.value())
        return;

    *tmp_node = *(it.value());
}

QModelIndex TreeModel::GetIndex(int node_id) const
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

void TreeModel::NodeList(QList<const Node*>& node_list, const QList<int>& id_list) const
{
    node_list.reserve(id_list.size());

    for (auto node_id : id_list) {
        auto it { node_hash_.constFind(node_id) };
        if (it != node_hash_.constEnd() && it.value()) {
            node_list.emplaceBack(it.value());
        }
    }
}

bool TreeModel::ChildrenEmpty(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    return (it == node_hash_.constEnd()) ? true : it.value()->children.isEmpty();
}

void TreeModel::ConstructTree(Node* root)
{
    sql_->BuildTree(node_hash_);

    for (const auto& node : std::as_const(node_hash_)) {
        if (!node->parent) {
            node->parent = root;
            root->children.emplace_back(node);
        }
    }

    QString path {};
    for (auto& node : std::as_const(node_hash_)) {
        path = ConstructPath(node, root);

        if (node->branch) {
            branch_path_.insert(node->id, path);
            continue;
        }

        UpdateBranchTotal(node, root, node->initial_total, node->final_total);
        leaf_path_.insert(node->id, path);
    }

    node_hash_.insert(-1, root);
}

bool TreeModel::IsDescendant(Node* lhs, Node* rhs) const
{
    if (!lhs || !rhs || lhs == rhs)
        return false;

    while (lhs && lhs != rhs)
        lhs = lhs->parent;

    return lhs == rhs;
}

QString TreeModel::ConstructPath(const Node* node, const Node* root) const
{
    if (!node || node == root)
        return QString();

    QStringList tmp {};

    while (node && node != root) {
        tmp.prepend(node->name);
        node = node->parent;
    }

    return tmp.join(separator_);
}

void TreeModel::UpdatePath(const Node* node, const Node* root)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};
    QString path {};

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        path = ConstructPath(current, root);

        if (current->branch) {
            for (const auto& child : current->children)
                queue.enqueue(child);

            branch_path_.insert(current->id, path);
            continue;
        }

        leaf_path_.insert(current->id, path);
    }
}

void TreeModel::SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare)
{
    if (!node)
        return;

    QQueue<Node*> queue {};
    queue.enqueue(node);

    Node* current {};

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        if (current->children.isEmpty())
            continue;

        std::sort(current->children.begin(), current->children.end(), Compare);
        for (const auto& child : current->children) {
            queue.enqueue(child);
        }
    }
}

void TreeModel::UpdateBranchTotal(const Node* node, const Node* root, double initial_diff, double final_diff)
{
    if (!node)
        return;

    bool equal {};
    const int unit { node->unit };
    const bool node_rule { node->node_rule };

    while (node && node != root) {
        equal = node->parent->node_rule == node_rule;
        node->parent->final_total += (equal ? 1 : -1) * final_diff;

        if (node->parent->unit == unit)
            node->parent->initial_total += (equal ? 1 : -1) * initial_diff;

        node = node->parent;
    }
}

bool TreeModel::UpdateLeafTotal(const Node* node, CString& initial, CString& final)
{
    if (!node || node->branch)
        return false;

    auto node_id { node->id };

    sql_->UpdateField(info_.node, initial, node->initial_total, node_id);
    if (!final.isEmpty())
        sql_->UpdateField(info_.node, final, node->final_total, node_id);

    return true;
}

bool TreeModel::UpdateCode(Node* node, CString& new_value, CString& field) { return UpdateField(node, new_value, field, &Node::code); }

bool TreeModel::UpdateDescription(Node* node, CString& new_value, CString& field) { return UpdateField(node, new_value, field, &Node::description); }

bool TreeModel::UpdateNote(Node* node, CString& new_value, CString& field) { return UpdateField(node, new_value, field, &Node::note); }

bool TreeModel::UpdateBranch(Node* node, bool value)
{
    int node_id { node->id };
    if (node->branch == value || !node->children.isEmpty() || table_hash_.contains(node_id))
        return false;

    if (sql_->NodeInternalReferences(node_id) || sql_->NodeExternalReferences(node_id))
        return false;

    node->branch = value;
    sql_->UpdateField(info_.node, BRANCH, value, node_id);

    (node->branch) ? branch_path_.insert(node_id, leaf_path_.take(node_id)) : leaf_path_.insert(node_id, branch_path_.take(node_id));
    return true;
}

void TreeModel::UpdateSeparator(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    auto UpdatePaths = [&old_separator, &new_separator](auto& paths) {
        for (auto& path : paths)
            path.replace(old_separator, new_separator);
    };

    UpdatePaths(leaf_path_);
    UpdatePaths(branch_path_);
}

bool TreeModel::UpdateNodeRule(Node* node, bool value)
{
    if (node->node_rule == value)
        return false;

    node->node_rule = value;
    sql_->UpdateField(info_.node, NODE_RULE, value, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    if (!node->branch) {
        emit SNodeRule(node->id, value);
        UpdateLeafTotal(node, INITIAL_TOTAL, FINAL_TOTAL);
    }

    return true;
}

Node* TreeModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Node*>(index.internalPointer());

    return root_;
}

bool TreeModel::UpdateUnit(Node* node, int value)
{
    int node_id { node->id };
    if (node->unit == value || sql_->NodeInternalReferences(node_id) || sql_->NodeExternalReferences(node_id))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, UNIT, value, node_id);

    if (node->branch)
        UpdateBranchUnit(node);

    return true;
}

QMimeData* TreeModel::mimeData(const QModelIndexList& indexes) const
{
    auto mime_data { new QMimeData() };
    if (indexes.isEmpty())
        return mime_data;

    auto first_index { indexes.first() };

    if (first_index.isValid()) {
        int id { first_index.sibling(first_index.row(), std::to_underlying(TreeEnum::kID)).data().toInt() };
        mime_data->setData(NODE_ID, QByteArray::number(id));
    }

    return mime_data;
}

void TreeModel::UpdateBranchUnit(Node* node) const
{
    if (!node || !node->branch || node->unit == root_->unit)
        return;

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};

    double initial_total { 0.0 };
    const int unit { node->unit };
    const bool node_rule { node->node_rule };

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        if (current->branch) {
            for (const auto& child : current->children)
                queue.enqueue(child);
        } else if (current->unit == unit) {
            initial_total += (current->node_rule == node_rule ? 1 : -1) * current->initial_total;
        }
    }

    node->initial_total = initial_total;
}

bool TreeModel::UpdateName(Node* node, Node* root, CString& new_value)
{
    node->name = new_value;
    sql_->UpdateField(info_.node, NAME, new_value, node->id);

    UpdatePath(node, root);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}

void TreeModel::InitializeRoot(Node*& root, int base_unit)
{
    if (root == nullptr) {
        root = ResourcePool<Node>::Instance().Allocate();
        root->id = -1;
        root->branch = true;
        root->unit = base_unit;
    }

    assert(root != nullptr && "Root node should not be null after initialization");
}
