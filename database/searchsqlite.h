#ifndef SEARCHSQLITE_H
#define SEARCHSQLITE_H

#include <QSqlDatabase>

#include "component/info.h"
#include "component/using.h"
#include "table/transaction.h"

class SearchSqlite {
public:
    SearchSqlite(CInfo& info, QHash<int, Transaction*>* transaction_hash);

    QList<int> Node(CString& text);
    TransactionList TransList(CString& text);

private:
    QSqlDatabase* db_ {};
    QHash<int, Transaction*>* transaction_hash_ {};
    CInfo& info_;
};

#endif // SEARCHSQLITE_H
