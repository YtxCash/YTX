#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include <QDate>

#include "database/sqlite/sqliteorder.h"
#include "tree/model/treemodel.h"

class TreeModelOrder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelOrder() override = default;

signals:
    void SUpdateData(int node_id, TreeEnumOrder column, const QVariant& value);

public slots:
    void RUpdateLeafValueOne(int node_id, double diff, CString& node_field) override; // first
    void RUpdateLeafValue(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff) override;

    bool RUpdateStakeholderReference(int old_node_id, int new_node_id);
    void RUpdateLocked(int node_id, bool checked);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;

    void ConstructTreeOrder(const QDate& start_date, const QDate& end_date);

protected:
    bool UpdateRule(Node* node, bool value) override; // charge = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateName(Node* node, CString& value) override;
    void UpdateAncestorValue(
        Node* node, double first_diff, double second_diff = 0.0, double amount_diff = 0.0, double discount_diff = 0.0, double settled_diff = 0.0) override;

private:
    bool UpdateLocked(Node* node, bool value);

private:
    SqliteOrder* sql_ {};
};

#endif // TREEMODELORDER_H
