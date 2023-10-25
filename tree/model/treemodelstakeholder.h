#ifndef TREEMODELSTAKEHOLDER_H
#define TREEMODELSTAKEHOLDER_H

#include "tree/model/treemodel.h"
#include "widget/tablewidget.h"

class TreeModelStakeholder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelStakeholder(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelStakeholder() override = default;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    void UpdateNode(const Node* tmp_node) override;
    void UpdateBaseUnit(int base_unit) override { root_->unit = base_unit; }

protected:
    bool UpdateNodeRule(Node* node, bool value) override;

    bool UpdatePaymentPeriod(Node* node, int value);
    bool UpdateTaxRate(Node* node, double value);
    bool UpdateDeadline(Node* node, CString& value);
    bool UpdateEmployee(Node* node, int value);
};

#endif // TREEMODELSTAKEHOLDER_H
