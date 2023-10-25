#include "sqlconnectionpool.h"

SqlConnectionPool& SqlConnectionPool::Instance()
{
    static SqlConnectionPool instance;
    return instance;
}

QSqlDatabase SqlConnectionPool::Allocate()
{
    QMutexLocker locker(&mutex_);
    if (!is_initialized_) {
        auto db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("test.db");
        is_initialized_ = true;
    }

    if (pool_.isEmpty()) {
        return CreateConnection();
    }

    QSqlDatabase connection = pool_.dequeue();
    if (IsValidConnection(connection)) {
        return connection;
    }

    return CreateConnection();
}

void SqlConnectionPool::Recycle(QSqlDatabase& connection)
{
    if (!connection.isValid()) {
        return;
    }

    QMutexLocker locker(&mutex_);
    pool_.enqueue(connection);
}

SqlConnectionPool::SqlConnectionPool()
    : is_initialized_ { false }
{
}

QSqlDatabase SqlConnectionPool::CreateConnection()
{
    auto connection = QSqlDatabase::database();

    if (!connection.isValid()) {
        connection = QSqlDatabase::addDatabase("QSQLITE");
        connection.setDatabaseName("test.db");
    }

    return connection.open() ? connection : QSqlDatabase();
}

bool SqlConnectionPool::IsValidConnection(QSqlDatabase connection)
{
    return connection.isValid() && connection.isOpen();
}
