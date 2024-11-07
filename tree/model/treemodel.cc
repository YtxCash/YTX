#include "treemodel.h"

#include <QQueue>

TreeModel::TreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

void TreeModel::RRemoveNode(int node_id)
{
    auto index { GetIndex(node_id) };
    int row { index.row() };
    auto parent_index { index.parent() };
    RemoveNode(row, parent_index);
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    // root_'s index is QModelIndex(), root_'s id == -1
    if (!index.isValid())
        return QModelIndex();

    auto* node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QModelIndex();

    auto* parent_node { node->parent };
    if (parent_node->id == -1)
        return QModelIndex();

    return createIndex(parent_node->parent->children.indexOf(parent_node), 0, parent_node);
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    return node ? createIndex(row, column, node) : QModelIndex();
}

int TreeModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

QMimeData* TreeModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mime_data { new QMimeData() };
    if (indexes.isEmpty())
        return mime_data;

    auto first_index { indexes.first() };

    if (first_index.isValid()) {
        int id { first_index.sibling(first_index.row(), std::to_underlying(TreeEnum::kID)).data().toInt() };
        mime_data->setData(NODE_ID, QByteArray::number(id));
    }

    return mime_data;
}

QSet<int> TreeModel::ChildrenSetFPTS(const Node* node)
{
    if (!node->branch || node->children.isEmpty())
        return {};

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    QSet<int> set {};
    while (!queue.isEmpty()) {
        auto* queue_node = queue.dequeue();

        if (queue_node->branch)
            for (const auto* child : queue_node->children)
                queue.enqueue(child);
        else
            set.insert(queue_node->id);
    }

    return set;
}
