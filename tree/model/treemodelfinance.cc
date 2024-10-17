#include "treemodelfinance.h"

TreeModelFinance::TreeModelFinance(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    ConstructTree();
}
