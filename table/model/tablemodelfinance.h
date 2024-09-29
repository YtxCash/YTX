#ifndef TABLEMODELFINANCE_H
#define TABLEMODELFINANCE_H

#include "tablemodel.h"

class TableModelFinance final : public TableModel {
    Q_OBJECT

public:
    TableModelFinance(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelFinance() override = default;
};

#endif // TABLEMODELFINANCE_H
