#ifndef TREEMODELTASK_H
#define TREEMODELTASK_H

#include "treemodel.h"

class TreeModelTask : public TreeModel {
    Q_OBJECT

public:
    TreeModelTask(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelTask() = default;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
};

#endif // TREEMODELTASK_H
