#include "tablemodelfinance.h"

TableModelFinance::TableModelFinance(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
{
}
