#ifndef SQLCONNECTIONPOOL_H
#define SQLCONNECTIONPOOL_H

#include <QMutex>
#include <QQueue>
#include <QSqlDatabase>

class SqlConnectionPool {
public:
    static SqlConnectionPool& Instance();
    QSqlDatabase Allocate();
    void Recycle(QSqlDatabase& connection);

private:
    SqlConnectionPool();
    QSqlDatabase CreateConnection();
    bool IsValidConnection(QSqlDatabase connection);

private:
    QMutex mutex_;
    QQueue<QSqlDatabase> pool_;
    bool is_initialized_ { false };
};

#endif // SQLCONNECTIONPOOL_H
