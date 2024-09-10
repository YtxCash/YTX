#ifndef TREEMODELFINANCETASK_H
#define TREEMODELFINANCETASK_H

#include "abstracttreemodel.h"

class TreeModelFinanceTask : public AbstractTreeModel {
    Q_OBJECT

public:
    TreeModelFinanceTask(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelFinanceTask() = default;
};

#endif // TREEMODELFINANCETASK_H
