#include "treemodel.h"

#include <QApplication>
#include <QFutureWatcher>
#include <QQueue>
#include <QTimer>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "widget/temporarylabel.h"

TreeModel::TreeModel(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    InitializeRoot(default_unit);
}

TreeModel::~TreeModel() { qDeleteAll(node_hash_); }

bool TreeModel::RUpdateMultiLeafTotal(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_diff {};
    double initial_diff {};
    Node* node {};

    for (int node_id : node_list) {
        node = GetNodeByID(node_id);

        if (!node || node->branch)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->LeafTotal(node);
        sql_->UpdateNodeValue(node);

        final_diff = node->final_total - old_final_total;
        initial_diff = node->initial_total - old_initial_total;

        UpdateAncestorValue(node, initial_diff, final_diff);
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

void TreeModel::RUpdateLeafValue(
    int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double /*settled_diff*/)
{
    auto node { GetNodeByID(node_id) };
    if (!node || node == root_ || node->branch)
        return;

    if (initial_credit_diff == 0 && initial_debit_diff == 0 && final_debit_diff == 0 && final_credit_diff == 0)
        return;

    auto rule { node->rule };

    auto initial_diff { (rule ? 1 : -1) * (initial_credit_diff - initial_debit_diff) };
    auto final_diff { (rule ? 1 : -1) * (final_credit_diff - final_debit_diff) };

    node->initial_total += initial_diff;
    node->final_total += final_diff;

    sql_->UpdateNodeValue(node);
    UpdateAncestorValue(node, initial_diff, final_diff);
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
        for (auto* child : node->children) {
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
        UpdateAncestorValue(node, -node->initial_total, -node->final_total);
        leaf_path_.remove(node_id);
        sql_->RemoveNode(node_id, false);
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumCommon::kName));
    emit SUpdateComboModel();

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

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { ConstructPath(node) };
    (node->branch ? branch_path_ : leaf_path_).insert(node->id, path);

    emit SSearch();
    emit SUpdateComboModel();
    return true;
}

void TreeModel::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto it { node_hash_.constFind(tmp_node->id) };
    if (it == node_hash_.constEnd())
        return;

    auto node { it.value() };
    if (*node == *tmp_node)
        return;

    UpdateBranch(node, tmp_node->branch);
    UpdateRule(node, tmp_node->rule);
    UpdateUnit(node, tmp_node->unit);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node);
        emit SUpdateComboModel();
    }

    UpdateField(node, tmp_node->description, DESCRIPTION, &Node::description);
    UpdateField(node, tmp_node->code, CODE, &Node::code);
    UpdateField(node, tmp_node->note, NOTE, &Node::note);
}

void TreeModel::UpdateBaseUnit(int default_unit)
{
    if (root_->unit == default_unit)
        return;

    root_->unit = default_unit;

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash)
        if (node->branch && node->unit != default_unit)
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

    const TreeEnum kColumn { index.column() };

    switch (kColumn) {
    case TreeEnum::kName:
        return node->name;
    case TreeEnum::kID:
        return node->id;
    case TreeEnum::kCode:
        return node->code;
    case TreeEnum::kDescription:
        return node->description;
    case TreeEnum::kNote:
        return node->note;
    case TreeEnum::kRule:
        return node->rule;
    case TreeEnum::kBranch:
        return node->branch ? node->branch : QVariant();
    case TreeEnum::kUnit:
        return node->unit;
    case TreeEnum::kInitialTotal:
        return node->unit == root_->unit ? QVariant() : node->initial_total;
    case TreeEnum::kFinalTotal:
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

    const TreeEnum kColumn { index.column() };

    switch (kColumn) {
    case TreeEnum::kCode:
        UpdateField(node, value.toString(), CODE, &Node::code);
        break;
    case TreeEnum::kDescription:
        UpdateField(node, value.toString(), DESCRIPTION, &Node::description);
        break;
    case TreeEnum::kNote:
        UpdateField(node, value.toString(), NOTE, &Node::note);
        break;
    case TreeEnum::kRule:
        UpdateRule(node, value.toBool());
        break;
    case TreeEnum::kBranch:
        UpdateBranch(node, value.toBool());
        break;
    case TreeEnum::kUnit:
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
        const TreeEnum kColumn { column };
        switch (kColumn) {
        case TreeEnum::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnum::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnum::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnum::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnum::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnum::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnum::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnum::kFinalTotal:
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
    const TreeEnum kColumn { index.column() };

    switch (kColumn) {
    case TreeEnum::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnum::kInitialTotal:
    case TreeEnum::kFinalTotal:
    case TreeEnum::kBranch:
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
        UpdateAncestorValue(node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        UpdateAncestorValue(node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    UpdatePath(node);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumCommon::kName));
    emit SUpdateName(node);
    emit SUpdateComboModel();

    return true;
}

void TreeModel::LeafPathSpecificUnit(QStandardItemModel* combo_model, int unit, UnitFilterMode unit_filter_mode) const
{
    if (!combo_model)
        return;

    auto future = QtConcurrent::run([this, unit, unit_filter_mode]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf_path_.size());

        auto should_add = [unit, unit_filter_mode](const Node* node) {
            switch (unit_filter_mode) {
            case UnitFilterMode::kIncludeUnitOnlyWithEmpty:
            case UnitFilterMode::kIncludeUnitOnly:
                return node->unit == unit;
            case UnitFilterMode::kExcludeUnitOnly:
                return node->unit != unit;
            default:
                return true;
            }
        };

        if (unit_filter_mode == UnitFilterMode::kIncludeUnitOnlyWithEmpty) {
            items.emplaceBack(QString(), 0);
        }

        for (const auto& [id, path] : leaf_path_.asKeyValueRange()) {
            auto it = node_hash_.constFind(id);
            if (it != node_hash_.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(combo_model);
    connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, this, [this, watcher, combo_model]() {
        UpdateComboModel(combo_model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModel::LeafPathExcludeID(QStandardItemModel* combo_model, int exclude_id) const
{
    if (!combo_model)
        return;

    auto future = QtConcurrent::run([this, exclude_id]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf_path_.size());

        for (const auto& [id, path] : leaf_path_.asKeyValueRange()) {
            if (id != exclude_id)
                items.emplaceBack(path, id);
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(combo_model);
    connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, this, [this, watcher, combo_model]() {
        UpdateComboModel(combo_model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModel::LeafPathBranchPath(QStandardItemModel* combo_model) const
{
    if (!combo_model)
        return;

    auto future = QtConcurrent::run([this]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf_path_.size() + branch_path_.size());

        for (const auto& [id, path] : leaf_path_.asKeyValueRange())
            items.emplaceBack(path, id);

        for (const auto& [id, path] : branch_path_.asKeyValueRange())
            items.emplaceBack(path, id);

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(combo_model);
    connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, this, [this, watcher, combo_model]() {
        UpdateComboModel(combo_model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModel::UpdateComboModel(QStandardItemModel* combo_model, const QVector<std::pair<QString, int>>& items) const
{
    if (!combo_model || items.isEmpty())
        return;

    combo_model->clear();
    QSignalBlocker blocker(combo_model);

    for (const auto& item : items) {
        auto standard_item = new QStandardItem(item.first);
        standard_item->setData(item.second, Qt::UserRole);
        combo_model->appendRow(standard_item);
    }

    combo_model->sort(0);
}

QStringList TreeModel::ChildrenName(int node_id, int exclude_child) const
{
    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return {};

    auto node { it.value() };
    QStringList list { node->children.size() };

    for (const auto* child : node->children) {
        if (child->id != exclude_child)
            list.emplaceBack(child->name);
    }

    return list;
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

void TreeModel::SetNodeShadow(NodeShadow* node_shadow, int node_id) const
{
    if (!node_shadow || node_id <= 0)
        return;

    auto it { node_hash_.constFind(node_id) };
    if (it != node_hash_.constEnd() && it.value())
        node_shadow->Set(it.value());
}

void TreeModel::SetNodeShadow(NodeShadow* node_shadow, Node* node) const
{
    if (!node_shadow || !node)
        return;

    node_shadow->Set(node);
}

void TreeModel::SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const
{
    node_list.reserve(node_id_list.size());

    for (int node_id : node_id_list) {
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

QString TreeModel::GetPath(int node_id) const
{
    if (auto it = leaf_path_.constFind(node_id); it != leaf_path_.constEnd())
        return it.value();

    if (auto it = branch_path_.constFind(node_id); it != branch_path_.constEnd())
        return it.value();

    return {};
}

bool TreeModel::IsDescendant(Node* lhs, Node* rhs) const
{
    if (!lhs || !rhs || lhs == rhs)
        return false;

    while (lhs && lhs != rhs)
        lhs = lhs->parent;

    return lhs == rhs;
}

QString TreeModel::ConstructPath(const Node* node) const
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

void TreeModel::UpdatePath(const Node* node)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};
    QString path {};

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        path = ConstructPath(current);

        if (current->branch) {
            for (const auto* child : current->children)
                queue.enqueue(child);

            branch_path_.insert(current->id, path);
            continue;
        }

        leaf_path_.insert(current->id, path);
    }
}

void TreeModel::SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare) const
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
        for (auto* child : current->children) {
            queue.enqueue(child);
        }
    }
}

Node* TreeModel::GetNodeByID(int node_id) const
{
    if (auto it = node_hash_.constFind(node_id); it != node_hash_.constEnd())
        return it.value();

    return nullptr;
}

void TreeModel::UpdateAncestorValue(
    Node* node, double initial_diff, double final_diff, double /*amount_diff*/, double /*discount_diff*/, double /*settled_diff*/)
{
    if (!node || node == root_ || node->parent == root_ || !node->parent)
        return;

    if (initial_diff == 0 && final_diff == 0)
        return;

    bool equal {};
    const int unit { node->unit };
    const bool rule { node->rule };

    for (node = node->parent; node && node != root_; node = node->parent) {
        equal = node->rule == rule;
        node->final_total += (equal ? 1 : -1) * final_diff;

        if (node->unit == unit)
            node->initial_total += (equal ? 1 : -1) * initial_diff;
    }
}

bool TreeModel::UpdateBranch(Node* node, bool value)
{
    if (node->branch == value)
        return false;

    const int node_id { node->id };
    const QString path { GetPath(node_id) };
    QString message {};

    message = tr("Cannot change %1 branch,").arg(path);
    if (HasChildren(node, message))
        return false;

    message = tr("Cannot change %1 branch,").arg(path);
    if (IsOpened(node_id, message))
        return false;

    message = tr("Cannot change %1 branch,").arg(path);
    if (IsReferenced(node_id, message))
        return false;

    node->branch = value;
    sql_->UpdateField(info_.node, value, BRANCH, node_id);

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

void TreeModel::ConstructTree()
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
        path = ConstructPath(node);

        if (node->branch) {
            branch_path_.insert(node->id, path);
            continue;
        }

        UpdateAncestorValue(node, node->initial_total, node->final_total);
        leaf_path_.insert(node->id, path);
    }

    node_hash_.insert(-1, root_);
}

bool TreeModel::UpdateRule(Node* node, bool value)
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

Node* TreeModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Node*>(index.internalPointer());

    return root_;
}

bool TreeModel::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    int node_id { node->id };
    auto message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };
    if (IsReferenced(node_id, message))
        return false;

    node->unit = value;
    sql_->UpdateField(info_.node, value, UNIT, node_id);

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
        int id { first_index.sibling(first_index.row(), std::to_underlying(TreeEnumCommon::kID)).data().toInt() };
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
    const bool rule { node->rule };

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        if (current->branch) {
            for (const auto* child : current->children)
                queue.enqueue(child);
        } else if (current->unit == unit) {
            initial_total += (current->rule == rule ? 1 : -1) * current->initial_total;
        }
    }

    node->initial_total = initial_total;
}

bool TreeModel::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, NAME, node->id);

    UpdatePath(node);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumCommon::kName));
    emit SSearch();
    return true;
}

void TreeModel::InitializeRoot(int default_unit)
{
    if (root_ == nullptr) {
        root_ = ResourcePool<Node>::Instance().Allocate();
        root_->id = -1;
        root_->branch = true;
        root_->unit = default_unit;
    }

    assert(root_ != nullptr && "Root node should not be null after initialization");
}

void TreeModel::ShowTemporaryTooltip(CString& message, int duration) const
{
    auto label { new TemporaryLabel(message) };
    label->setWindowFlags(Qt::ToolTip);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setWordWrap(true);
    label->setAttribute(Qt::WA_DeleteOnClose);
    label->adjustSize();

    const int extra_space = 12 * 2;
    label->resize(label->width() + extra_space, label->height() + extra_space);
    label->setContentsMargins(12, 0, 0, 0);

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

    connect(label, &QLabel::destroyed, this, [timer]() { timer->stop(); });
}

bool TreeModel::HasChildren(Node* node, CString& message) const
{
    if (!node->children.isEmpty()) {
        ShowTemporaryTooltip(tr("%1 it has children nodes.").arg(message), 3000);
        return true;
    }

    return false;
}

bool TreeModel::IsOpened(int node_id, CString& message) const
{
    if (table_hash_.contains(node_id)) {
        ShowTemporaryTooltip(tr("%1 it is opened.").arg(message), 3000);
        return true;
    }

    return false;
}

bool TreeModel::IsReferenced(int node_id, CString& message) const
{
    if (sql_->InternalReference(node_id)) {
        ShowTemporaryTooltip(tr("%1 it is internal referenced.").arg(message), 3000);
        return true;
    }

    return false;
}
