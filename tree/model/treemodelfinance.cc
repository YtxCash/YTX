#include "treemodelfinance.h"

TreeModelFinance::TreeModelFinance(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, base_unit, table_hash, separator, parent)
{
    ConstructTree();
}
