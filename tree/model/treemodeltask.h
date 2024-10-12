#ifndef TREEMODELTASK_H
#define TREEMODELTASK_H

#include "treemodel.h"

class TreeModelTask : public TreeModel {
    Q_OBJECT

public:
    TreeModelTask(SPSqlite sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelTask() = default;

public slots:
    void RUpdateLeafValueOne(int node_id, double diff) override; // unit_cost

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void UpdateNode(const Node* tmp_node) override;
};

#endif // TREEMODELTASK_H
