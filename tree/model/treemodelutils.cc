#include "treemodelutils.h"

#include <QApplication>
#include <QQueue>
#include <QtConcurrent>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "widget/temporarylabel.h"

void TreeModelUtils::UpdateBranchUnitF(const Node* root, Node* node)
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

void TreeModelUtils::UpdatePathFPTS(StringHash& leaf, StringHash& branch, const Node* root, const Node* node, CString& separator)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    const Node* current {};
    QString path {};

    while (!queue.isEmpty()) {
        current = queue.dequeue();

        path = ConstructPathFPTS(root, current, separator);

        if (current->branch) {
            for (const auto* child : current->children)
                queue.enqueue(child);

            branch.insert(current->id, path);
            continue;
        }

        leaf.insert(current->id, path);
    }
}

void TreeModelUtils::InitializeRoot(Node*& root, int default_unit)
{
    if (root == nullptr) {
        root = ResourcePool<Node>::Instance().Allocate();
        root->id = -1;
        root->branch = true;
        root->unit = default_unit;
    }

    assert(root != nullptr && "Root node should not be null after initialization");
}

Node* TreeModelUtils::GetNodeByID(CNodeHash& hash, int node_id)
{
    if (auto it = hash.constFind(node_id); it != hash.constEnd())
        return it.value();

    return nullptr;
}

bool TreeModelUtils::IsDescendant(Node* lhs, Node* rhs)
{
    if (!lhs || !rhs || lhs == rhs)
        return false;

    while (lhs && lhs != rhs)
        lhs = lhs->parent;

    return lhs == rhs;
}

void TreeModelUtils::SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare)
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

QString TreeModelUtils::ConstructPathFPTS(const Node* root, const Node* node, CString& separator)
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

void TreeModelUtils::ShowTemporaryTooltipFPTS(CString& message, int duration)
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

bool TreeModelUtils::IsInternalReferencedFPTS(Sqlite* sql, int node_id, CString& message)
{
    if (sql->InternalReference(node_id)) {
        TreeModelUtils::ShowTemporaryTooltipFPTS(QObject::tr("%1 it is internal referenced.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelUtils::IsHelperReferencedFPTS(Sqlite* sql, int node_id, CString& message)
{
    if (sql->HelperReferenceFPTS(node_id)) {
        TreeModelUtils::ShowTemporaryTooltipFPTS(QObject::tr("%1 it is helper referenced.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelUtils::IsExternalReferencedPS(Sqlite* sql, int node_id, CString& message)
{
    if (sql->ExternalReference(node_id)) {
        TreeModelUtils::ShowTemporaryTooltipFPTS(QObject::tr("%1 it is external referenced.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelUtils::HasChildrenFPTS(Node* node, CString& message)
{
    if (!node->children.isEmpty()) {
        ShowTemporaryTooltipFPTS(QObject::tr("%1 it has children nodes.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelUtils::IsBranchFPTS(Node* node, CString& message)
{
    if (node->branch) {
        ShowTemporaryTooltipFPTS(QObject::tr("%1 it is branch.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelUtils::IsHelperFPTS(Node* node, CString& message)
{
    if (node->is_helper) {
        ShowTemporaryTooltipFPTS(QObject::tr("%1 it is helper.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

bool TreeModelUtils::IsOpenedFPTS(CTableHash& hash, int node_id, CString& message)
{
    if (hash.contains(node_id)) {
        ShowTemporaryTooltipFPTS(QObject::tr("%1 it is opened.").arg(message), THREE_THOUSAND);
        return true;
    }

    return false;
}

void TreeModelUtils::UpdateComboModel(QStandardItemModel* model, const QVector<std::pair<QString, int>>& items)
{
    if (!model || items.isEmpty())
        return;

    model->clear();
    QSignalBlocker blocker(model);

    for (const auto& item : items) {
        auto* standard_item = new QStandardItem(item.first);
        standard_item->setData(item.second, Qt::UserRole);
        model->appendRow(standard_item);
    }

    model->sort(0);
}

void TreeModelUtils::PathPreferencesFPT(CNodeHash& hash, CStringHash& leaf, CStringHash& branch, QStandardItemModel* model)
{
    if (!model)
        return;

    auto future = QtConcurrent::run([&]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size() + branch.size());

        auto should_add = [](const Node* node) { return !node->is_helper; };

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        for (const auto& [id, path] : branch.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        items.emplaceBack(QString(), 0);

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        TreeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModelUtils::LeafPathRhsNodeFPT(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_node, Filter filter)
{
    if (!model)
        return;

    auto future = QtConcurrent::run([&, specific_node, filter]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size());

        auto should_add = [specific_node, filter](const Node* node) {
            switch (filter) {
            case Filter::kExcludeSpecific:
                return node->id != specific_node && !node->is_helper;
            default:
                return false;
            }
        };

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        TreeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModelUtils::LeafPathSpecificUnitPS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, Filter filter)
{
    if (!model)
        return;

    auto future = QtConcurrent::run([&, specific_unit, filter]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size());

        auto should_add = [specific_unit, filter](const Node* node) {
            switch (filter) {
            case Filter::kIncludeSpecificWithNone:
            case Filter::kIncludeSpecific:
                return node->unit == specific_unit;
            case Filter::kExcludeSpecific:
                return node->unit != specific_unit;
            default:
                return false;
            }
        };

        if (filter == Filter::kIncludeSpecificWithNone) {
            items.emplaceBack(QString(), 0);
        }

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        TreeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModelUtils::LeafPathRemoveNodeFPTS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, int exclude_node)
{
    if (!model)
        return;

    auto future = QtConcurrent::run([&, specific_unit, exclude_node]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size());

        auto should_add = [specific_unit, exclude_node](const Node* node) { return node->unit == specific_unit && node->id != exclude_node; };

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        TreeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModelUtils::LeafPathHelperNodeFTS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_node, Filter filter)
{
    if (!model)
        return;

    auto future = QtConcurrent::run([&, specific_node, filter]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size());

        auto should_add = [specific_node, filter](const Node* node) {
            switch (filter) {
            case Filter::kIncludeAllWithNone:
                return node->is_helper;
            case Filter::kExcludeSpecific:
                return node->is_helper && node->id != specific_node;
            default:
                return false;
            }
        };

        if (filter == Filter::kIncludeSpecificWithNone || filter == Filter::kIncludeAllWithNone) {
            items.emplaceBack(QString(), 0);
        }

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value())) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        TreeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void TreeModelUtils::UpdateAncestorValueFPT(QMutex& mutex, const Node* root, Node* node, double initial_diff, double final_diff)
{
    if (!node || node == root || node->parent == root || !node->parent)
        return;

    if (initial_diff == 0 && final_diff == 0)
        return;

    const int unit = node->unit;
    const bool rule = node->rule;

    // 确保所有数据都是通过值传递，避免悬空指针
    auto future = QtConcurrent::run([=, &mutex]() {
        // 使用 RAII 方式加锁
        QMutexLocker locker(&mutex);

        // 遍历并更新祖先节点
        for (Node* current = node->parent; current && current != root; current = current->parent) {
            bool equal = current->rule == rule;
            current->final_total += (equal ? 1 : -1) * final_diff;
            if (current->unit == unit) {
                current->initial_total += (equal ? 1 : -1) * initial_diff;
            }
        }
    });
}
