#include "transactionpool.h"

QSharedPointer<Transaction> TransactionPool::Allocate()
{
    QMutexLocker locker(&mutex_);

    if (remain_ < kExpandThreshold) {
        ExpandCapacity(kFixedSize);
        remain_ += kFixedSize;
    }

    --remain_;
    return QSharedPointer<Transaction>(pool_.dequeue());
}

void TransactionPool::Recycle(QSharedPointer<Transaction> transaction)
{
    if (!transaction)
        return;

    QMutexLocker locker(&mutex_);

    if (remain_ > kShrinkThreshold) {
        ShrinkCapacity(kFixedSize);
        remain_ = remain_ - kFixedSize;
    }

    transaction.data()->ResetToDefault();
    pool_.enqueue(transaction.data());
    ++remain_;
}

TransactionPool::TransactionPool()
{
    ExpandCapacity(kFixedSize);
    remain_ = kFixedSize;
}

void TransactionPool::ExpandCapacity(int size)
{
    for (int i = 0; i != size; ++i) {
        pool_.enqueue(new Transaction());
    }
}

void TransactionPool::ShrinkCapacity(int size)
{
    for (int i = 0; i != size; ++i) {
        delete pool_.dequeue();
    }
}

TransactionPool::~TransactionPool()
{
    QMutexLocker locker(&mutex_);
    qDeleteAll(pool_);
    pool_.clear();
}

TransactionPool& TransactionPool::Instance()
{
    static TransactionPool instance;
    return instance;
}
