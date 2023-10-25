#include "node.h"
#include <QQueue>

void Node::ResetToDefault()
{
    id = 0;
    name.clear();
    description.clear();
    rule = false;
    placeholder = false;
    parent = nullptr;
    children.clear();
    total = 0.0;
}

Node::Node(int id, const QString& name, const QString& description, double total, bool rule, bool placeholder)
    : id { id }
    , name { name }
    , description { description }
    , rule { rule }
    , placeholder { placeholder }
    , total { total }
    , parent { nullptr }

{
}

Node::~Node()
{
    QQueue<Node*> queue;
    queue.enqueue(this);

    while (!queue.isEmpty()) {
        auto node = queue.dequeue();

        for (auto child : node->children) {
            queue.enqueue(child);
        }

        delete node;
    }
}
