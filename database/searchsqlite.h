#ifndef SEARCHSQLITE_H
#define SEARCHSQLITE_H

#include <QSqlDatabase>

#include "component/info.h"
#include "component/using.h"
#include "table/trans.h"

class SearchSqlite {
public:
    SearchSqlite(CInfo& info, QHash<int, Trans*>* trans_hash);

    QList<int> Node(CString& text);
    TransList QueryTransList(CString& text);

private:
    QSqlDatabase* db_ {};
    QHash<int, Trans*>* trans_hash_ {};
    CInfo& info_;
};

#endif // SEARCHSQLITE_H
