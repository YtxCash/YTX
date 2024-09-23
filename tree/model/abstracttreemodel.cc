#include "abstracttreemodel.h"

#include <QApplication>
#include <QQueue>
#include <QTimer>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "widget/temporarylabel.h"

AbstractTreeModel::AbstractTreeModel(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    InitializeRoot(base_unit);
}

AbstractTreeModel::~AbstractTreeModel() { qDeleteAll(node_hash_); }

bool AbstractTreeModel::RUpdateMultiTotal(const QList<int>& node_list)
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

        RecalculateAncestor(node, initial_diff, final_diff);
    }

    emit SUpdateDSpinBox();
    return true;
}

bool AbstractTreeModel::RRemoveNode(int node_id)
{
    auto index { GetIndex(node_id) };
    auto row { index.row() };
    auto parent_index { index.parent() };
    RemoveNode(row, parent_index);

    return true;
}

void AbstractTreeModel::RUpdateOneTotal(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff)
{
    auto node { GetNodeByID(node_id) };
    auto node_rule { node->node_rule };

    auto initial_diff { (node_rule ? 1 : -1) * (initial_credit_diff - initial_debit_diff) };
    auto final_diff { (node_rule ? 1 : -1) * (final_credit_diff - final_debit_diff) };

    node->initial_total += initial_diff;
    node->final_total += final_diff;

    UpdateLeafTotal(node, INITIAL_TOTAL, FINAL_TOTAL);
    RecalculateAncestor(node, initial_diff, final_diff);
    emit SUpdateDSpinBox();
}

bool AbstractTreeModel::RemoveNode(int row, const QModelIndex& parent)
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
        UpdatePath(node);
        branch_path_.remove(node_id);
        sql_->RemoveNode(node_id, true);
        emit SUpdateName(node);
    }

    if (!branch) {
        RecalculateAncestor(node, -node->initial_total, -node->final_total);
        leaf_path_.remove(node_id);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

bool AbstractTreeModel::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->InsertNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { ConstructPath(node) };
    (node->branch ? branch_path_ : leaf_path_).insert(node->id, path);

    emit SSearch();
    emit SUpdateOrderPartyEmployee();
    return true;
}

void AbstractTreeModel::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { node_hash_.value(tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateBranch(node, tmp_node->branch);
    UpdateNodeRule(node, tmp_node->node_rule);
    UpdateUnit(node, tmp_node->unit);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node);
    }

    // update code, description, note
    *node = *tmp_node;
    sql_->UpdateNodeSimple(node);
}

void AbstractTreeModel::UpdateBaseUnit(int base_unit)
{
    if (root_->unit == base_unit)
        return;

    root_->unit = base_unit;

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto node : const_node_hash)
        if (node->branch && node->unit != base_unit)
            UpdateBranchUnit(node);
}

QModelIndex AbstractTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto parent_node { GetNodeByIndex(parent) };
    auto node { parent_node->children.at(row) };

    return node ? createIndex(row, column, node) : QModelIndex();
}

QModelIndex AbstractTreeModel::parent(const QModelIndex& index) const
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

int AbstractTreeModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

QVariant AbstractTreeModel::data(const QModelIndex& index, int role) const
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

bool AbstractTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
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

void AbstractTreeModel::sort(int column, Qt::SortOrder order)
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

Qt::ItemFlags AbstractTreeModel::flags(const QModelIndex& index) const
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
    case TreeEnumFinanceTask::kBranch:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool AbstractTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
        RecalculateAncestor(node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        RecalculateAncestor(node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    UpdatePath(node);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateName(node);

    return true;
}

void AbstractTreeModel::ComboPathUnit(QComboBox* combo, int unit) const
{
    if (!combo)
        return;

    combo->clear();

    int id {};
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (const auto* node : const_node_hash)
        if (node && node->unit == unit && !node->branch) {
            id = node->id;
            combo->addItem(leaf_path_.value(id), id);
        }
}

void AbstractTreeModel::ComboPathLeaf(QComboBox* combo, int exclude) const
{
    if (!combo)
        return;

    for (const auto& [id, path] : leaf_path_.asKeyValueRange()) {
        if (id != exclude)
            combo->addItem(path, id);
    }
}

void AbstractTreeModel::ComboPathLeafBranch(QComboBox* combo) const
{
    if (!combo)
        return;

    for (const auto& [id, path] : leaf_path_.asKeyValueRange())
        combo->addItem(path, id);

    for (const auto& [id, path] : branch_path_.asKeyValueRange())
        combo->addItem(path, id);
}

QStringList AbstractTreeModel::ChildrenName(int node_id, int exclude_child) const
{
    const Node* node { node_hash_.value(node_id, nullptr) };
    if (!node)
        return {};

    QStringList list {};

    for (const auto* child : node->children) {
        if (child->id != exclude_child)
            list.emplaceBack(child->name);
    }

    return list;
}

void AbstractTreeModel::SetParent(Node* node, int parent_id) const
{
    if (!node)
        return;

    auto it { node_hash_.constFind(parent_id) };

    node->parent = it == node_hash_.constEnd() ? nullptr : it.value();
}

void AbstractTreeModel::CopyNode(Node* tmp_node, int node_id) const
{
    if (!tmp_node)
        return;

    auto it = node_hash_.constFind(node_id);
    if (it == node_hash_.constEnd() || !it.value())
        return;

    *tmp_node = *(it.value());
}

QModelIndex AbstractTreeModel::GetIndex(int node_id) const
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

void AbstractTreeModel::NodeList(QList<const Node*>& node_list, const QList<int>& id_list) const
{
    node_list.reserve(id_list.size());

    for (auto node_id : id_list) {
        auto it { node_hash_.constFind(node_id) };
        if (it != node_hash_.constEnd() && it.value()) {
            node_list.emplaceBack(it.value());
        }
    }
}

bool AbstractTreeModel::ChildrenEmpty(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    return (it == node_hash_.constEnd()) ? true : it.value()->children.isEmpty();
}

bool AbstractTreeModel::IsDescendant(Node* lhs, Node* rhs) const
{
    if (!lhs || !rhs || lhs == rhs)
        return false;

    while (lhs && lhs != rhs)
        lhs = lhs->parent;

    return lhs == rhs;
}

QString AbstractTreeModel::ConstructPath(const Node* node) const
{
    if (!node || node == root_)
        return QString();

    QStringList tmp {};

    while (node && node != root_) {
        tmp.prepend(node->name);
        node = node->parent;
    }

    return tmp.join(separator_);
}

void AbstractTreeModel::UpdatePath(const Node* node)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};
    QString path {};

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        path = ConstructPath(current);

        if (current->branch) {
            for (const auto& child : current->children)
                queue.enqueue(child);

            branch_path_.insert(current->id, path);
            continue;
        }

        leaf_path_.insert(current->id, path);
    }
}

void AbstractTreeModel::SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare)
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

void AbstractTreeModel::RecalculateAncestor(Node* node, double initial_diff, double final_diff)
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return;

    bool equal {};
    const int unit { node->unit };
    const bool node_rule { node->node_rule };

    for (node = node->parent; node && node != root_; node = node->parent) {
        equal = node->node_rule == node_rule;
        node->final_total += (equal ? 1 : -1) * final_diff;

        if (node->unit == unit)
            node->initial_total += (equal ? 1 : -1) * initial_diff;
    }
}

bool AbstractTreeModel::UpdateLeafTotal(const Node* node, CString& initial_field, CString& final_field)
{
    if (!node || node->branch)
        return false;

    auto node_id { node->id };

    sql_->UpdateField(info_.node, node->initial_total, initial_field, node_id);
    if (!final_field.isEmpty())
        sql_->UpdateField(info_.node, node->final_total, final_field, node_id);

    return true;
}

bool AbstractTreeModel::UpdateCode(Node* node, CString& value, CString& field) { return UpdateField(node, value, field, &Node::code); }

bool AbstractTreeModel::UpdateDescription(Node* node, CString& value, CString& field) { return UpdateField(node, value, field, &Node::description); }

bool AbstractTreeModel::UpdateNote(Node* node, CString& value, CString& field) { return UpdateField(node, value, field, &Node::note); }

bool AbstractTreeModel::UpdateBranch(Node* node, bool value)
{
    if (node->branch == value)
        return false;

    int node_id { node->id };

    if (!node->children.isEmpty() || table_hash_.contains(node_id)) {
        ShowTemporaryTooltip(tr("Cannot change %1's branch. It might be opened or have children nodes.").arg(node->name), 3000);
        return false;
    }

    if (sql_->NodeInternalReferences(node_id) || sql_->NodeExternalReferences(node_id)) {
        ShowTemporaryTooltip(tr("Cannot change %1's branch due to internal or external references").arg(node->name), 3000);
        return false;
    }

    node->branch = value;
    sql_->UpdateField(info_.node, value, BRANCH, node_id);

    (node->branch) ? branch_path_.insert(node_id, leaf_path_.take(node_id)) : leaf_path_.insert(node_id, branch_path_.take(node_id));
    return true;
}

void AbstractTreeModel::UpdateSeparator(CString& old_separator, CString& new_separator)
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

void AbstractTreeModel::ConstructTree()
{
    sql_->BuildTree(node_hash_);
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (const auto& node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    QString path {};
    for (auto& node : const_node_hash) {
        path = ConstructPath(node);

        if (node->branch) {
            branch_path_.insert(node->id, path);
            continue;
        }

        RecalculateAncestor(node, node->initial_total, node->final_total);
        leaf_path_.insert(node->id, path);
    }

    node_hash_.insert(-1, root_);
}

bool AbstractTreeModel::UpdateNodeRule(Node* node, bool value)
{
    if (node->node_rule == value)
        return false;

    node->node_rule = value;
    sql_->UpdateField(info_.node, value, NODE_RULE, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    if (!node->branch) {
        emit SNodeRule(node->id, value);
        UpdateLeafTotal(node, INITIAL_TOTAL, FINAL_TOTAL);
    }

    return true;
}

Node* AbstractTreeModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Node*>(index.internalPointer());

    return root_;
}

bool AbstractTreeModel::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    int node_id { node->id };
    if (sql_->NodeInternalReferences(node_id) || sql_->NodeExternalReferences(node_id)) {
        ShowTemporaryTooltip(tr("Cannot change %1's unit due to internal or external references").arg(node->name), 3000);
        return false;
    }

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

    if (node->branch)
        UpdateBranchUnit(node);

    return true;
}

QMimeData* AbstractTreeModel::mimeData(const QModelIndexList& indexes) const
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

void AbstractTreeModel::UpdateBranchUnit(Node* node) const
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

bool AbstractTreeModel::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    UpdatePath(node);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    emit SUpdateOrderPartyEmployee();
    return true;
}

void AbstractTreeModel::InitializeRoot(int base_unit)
{
    if (root_ == nullptr) {
        root_ = ResourcePool<Node>::Instance().Allocate();
        root_->id = -1;
        root_->branch = true;
        root_->unit = base_unit;
    }

    assert(root_ != nullptr && "Root node should not be null after initialization");
}

void AbstractTreeModel::ShowTemporaryTooltip(CString& message, int duration)
{
    auto label { new TemporaryLabel(message) };
    label->setWindowFlags(Qt::ToolTip);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(300, 100);
    label->setWordWrap(true);
    label->setAttribute(Qt::WA_DeleteOnClose);

    QWidget* mainWindow { QApplication::activeWindow() };
    if (mainWindow) {
        QRect parent_rect { mainWindow->geometry() };
        QPoint center_point { parent_rect.center() - QPoint(label->width() / 2, label->height() / 2) };
        label->move(center_point);
    }

    label->show();

    QTimer* timer { new QTimer(label) };
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, label, &QLabel::close);
    timer->start(duration);

    connect(label, &QLabel::destroyed, [timer]() { timer->stop(); });
}
