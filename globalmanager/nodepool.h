#ifndef NODEPOOL_H
#define NODEPOOL_H

#include "../tree/node.h"
#include <QMutex>
#include <QQueue>

class NodePool {
public:
    static NodePool& Instance();
    Node* Allocate();
    void Recycle(Node* node);

private:
    NodePool();
    ~NodePool();

    void ExpandCapacity(int size);
    void ShrinkCapacity(int size);

private:
    QQueue<Node*> pool_ {};
    QMutex mutex_ {};

    const int kFixedSize = 50;
    const int kExpandThreshold = 10;
    const int kShrinkThreshold = 200;
    int remain_ { 0 };
};

#endif // NODEPOOL_H
