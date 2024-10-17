#ifndef TREEMODELFINANCE_H
#define TREEMODELFINANCE_H

#include "treemodel.h"

class TreeModelFinance : public TreeModel {
    Q_OBJECT

public:
    TreeModelFinance(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelFinance() = default;
};

#endif // TREEMODELFINANCE_H
