#include "treemodelhelper.h"

#include <QApplication>
#include <QQueue>
#include <QTimer>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "widget/temporarylabel.h"

QString TreeModelHelper::GetPath(CStringHash& leaf_path, CStringHash& branch_path, int node_id)
{
    if (auto it = leaf_path.constFind(node_id); it != leaf_path.constEnd())
        return it.value();

    if (auto it = branch_path.constFind(node_id); it != branch_path.constEnd())
        return it.value();

    return {};
}

void TreeModelHelper::UpdateBranchUnit(const Node* root, Node* node)
{
    if (!node || !node->branch || node->unit == root->unit)
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

void TreeModelHelper::UpdatePath(StringHash& leaf_path, StringHash& branch_path, const Node* root, const Node* node, CString& separator)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};
    QString path {};

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        path = ConstructPath(root, current, separator);

        if (current->branch) {
            for (const auto* child : current->children)
                queue.enqueue(child);

            branch_path.insert(current->id, path);
            continue;
        }

        leaf_path.insert(current->id, path);
    }
}

void TreeModelHelper::InitializeRoot(Node*& root, int default_unit)
{
    if (root == nullptr) {
        root = ResourcePool<Node>::Instance().Allocate();
        root->id = -1;
        root->branch = true;
        root->unit = default_unit;
    }

    assert(root != nullptr && "Root node should not be null after initialization");
}

Node* TreeModelHelper::GetNodeByID(const NodeHash& node_hash, int node_id)
{
    if (auto it = node_hash.constFind(node_id); it != node_hash.constEnd())
        return it.value();

    return nullptr;
}

bool TreeModelHelper::IsDescendant(Node* lhs, Node* rhs)
{
    if (!lhs || !rhs || lhs == rhs)
        return false;

    while (lhs && lhs != rhs)
        lhs = lhs->parent;

    return lhs == rhs;
}

void TreeModelHelper::SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare)
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

QString TreeModelHelper::ConstructPath(const Node* root, const Node* node, CString& separator)
{
    if (!node || node == root)
        return QString();

    QStringList tmp {};

    while (node && node != root) {
        tmp.prepend(node->name);
        node = node->parent;
    }

    return tmp.join(separator);
}

void TreeModelHelper::ShowTemporaryTooltip(CString& message, int duration)
{
    auto* label { new TemporaryLabel(message) };
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
    QObject::connect(timer, &QTimer::timeout, label, &QLabel::close);
    timer->start(duration);

    QObject::connect(label, &QLabel::destroyed, timer, [timer]() { timer->stop(); });
}

bool TreeModelHelper::HasChildren(Node* node, CString& message)
{
    if (!node->children.isEmpty()) {
        ShowTemporaryTooltip(QObject::tr("%1 it has children nodes.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelHelper::IsOpened(CTableHash& table_hash, int node_id, CString& message)
{
    if (table_hash.contains(node_id)) {
        ShowTemporaryTooltip(QObject::tr("%1 it is opened.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

void TreeModelHelper::SearchNode(const NodeHash& node_hash, QList<const Node*>& node_list, const QList<int>& node_id_list)
{
    node_list.reserve(node_id_list.size());

    for (int node_id : node_id_list) {
        auto it { node_hash.constFind(node_id) };
        if (it != node_hash.constEnd() && it.value()) {
            node_list.emplaceBack(it.value());
        }
    }
}

bool TreeModelHelper::ChildrenEmpty(const NodeHash& node_hash, int node_id)
{
    auto it { node_hash.constFind(node_id) };
    return (it == node_hash.constEnd()) ? true : it.value()->children.isEmpty();
}

bool TreeModelHelper::Contains(const NodeHash& node_hash, int node_id) { return node_hash.contains(node_id); }

void TreeModelHelper::UpdateSeparator(StringHash& leaf_path, StringHash& branch_path, CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    auto UpdatePaths = [&old_separator, &new_separator](auto& paths) {
        for (auto& path : paths)
            path.replace(old_separator, new_separator);
    };

    UpdatePaths(leaf_path);
    UpdatePaths(branch_path);
}

void TreeModelHelper::CopyNode(const NodeHash& node_hash, Node* tmp_node, int node_id)
{
    if (!tmp_node)
        return;

    auto it = node_hash.constFind(node_id);
    if (it == node_hash.constEnd() || !it.value())
        return;

    *tmp_node = *(it.value());
}

void TreeModelHelper::SetParent(const NodeHash& node_hash, Node* node, int parent_id)
{
    if (!node)
        return;

    auto it { node_hash.constFind(parent_id) };

    node->parent = it == node_hash.constEnd() ? nullptr : it.value();
}

QStringList TreeModelHelper::ChildrenName(const NodeHash& node_hash, int node_id, int exclude_child)
{
    auto it { node_hash.constFind(node_id) };
    if (it == node_hash.constEnd())
        return {};

    auto* node { it.value() };
    QStringList list {};
    list.reserve(node->children.size());

    for (const auto* child : node->children) {
        if (child->id != exclude_child)
            list.emplaceBack(child->name);
    }

    return list;
}

void TreeModelHelper::UpdateComboModel(QStandardItemModel* combo_model, const QVector<std::pair<QString, int>>& items)
{
    if (!combo_model || items.isEmpty())
        return;

    combo_model->clear();
    QSignalBlocker blocker(combo_model);

    for (const auto& item : items) {
        auto* standard_item = new QStandardItem(item.first);
        standard_item->setData(item.second, Qt::UserRole);
        combo_model->appendRow(standard_item);
    }

    combo_model->sort(0);
}
