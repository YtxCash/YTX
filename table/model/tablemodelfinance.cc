#include "tablemodelfinance.h"

TableModelFinance::TableModelFinance(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
}
