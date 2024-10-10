#ifndef TREEMODELFINANCE_H
#define TREEMODELFINANCE_H

#include "treemodel.h"

class TreeModelFinance : public TreeModel {
    Q_OBJECT

public:
    TreeModelFinance(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelFinance() = default;
};

#endif // TREEMODELFINANCE_H
