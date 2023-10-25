#ifndef TRANSACTIONPOOL_H
#define TRANSACTIONPOOL_H

#include "../table/transaction.h"
#include <QMutex>
#include <QQueue>
#include <QSharedPointer>

class TransactionPool {
public:
    static TransactionPool& Instance();
    QSharedPointer<Transaction> Allocate();
    void Recycle(QSharedPointer<Transaction> transaction);

private:
    TransactionPool();
    ~TransactionPool();

    void ExpandCapacity(int size);
    void ShrinkCapacity(int size);

private:
    QQueue<Transaction*> pool_ {};
    QMutex mutex_ {};

    const int kFixedSize = 100;
    const int kExpandThreshold = 10;
    const int kShrinkThreshold = 1000;
    int remain_ { 0 };
};

#endif // TRANSACTIONPOOL_H
