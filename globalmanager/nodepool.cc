#include "nodepool.h"

NodePool& NodePool::Instance()
{
    static NodePool instance;
    return instance;
}

Node* NodePool::Allocate()
{
    QMutexLocker locker(&mutex_);

    if (remain_ < kExpandThreshold) {
        ExpandCapacity(kFixedSize);
        remain_ += kFixedSize;
    }

    --remain_;
    return pool_.dequeue();
}

void NodePool::Recycle(Node* node)
{
    if (!node)
        return;

    QMutexLocker locker(&mutex_);

    if (remain_ > kShrinkThreshold) {
        ShrinkCapacity(kFixedSize);
        remain_ -= kFixedSize;
    }

    node->ResetToDefault();
    pool_.enqueue(node);
    ++remain_;
}

NodePool::NodePool()
{
    ExpandCapacity(kFixedSize);
    remain_ = kFixedSize;
}

NodePool::~NodePool()
{
    QMutexLocker locker(&mutex_);
    qDeleteAll(pool_);
    pool_.clear();
}

void NodePool::ExpandCapacity(int size)
{
    for (int i = 0; i != size; ++i) {
        pool_.enqueue(new Node());
    }
}

void NodePool::ShrinkCapacity(int size)
{
    for (int i = 0; i != size; ++i) {
        delete pool_.dequeue();
    }
}
