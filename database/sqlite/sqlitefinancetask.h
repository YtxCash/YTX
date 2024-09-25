#ifndef SQLITEFINANCETASK_H
#define SQLITEFINANCETASK_H

#include "sqlite.h"

class SqliteFinanceTask final : public Sqlite {
public:
    SqliteFinanceTask(CInfo& info, QObject* parent = nullptr);
};

#endif // SQLITEFINANCETASK_H
