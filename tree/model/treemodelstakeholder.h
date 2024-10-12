#ifndef TREEMODELSTAKEHOLDER_H
#define TREEMODELSTAKEHOLDER_H

#include "tree/model/treemodel.h"

class TreeModelStakeholder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelStakeholder(SPSqlite sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelStakeholder() override = default;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;
    void UpdateNode(const Node* tmp_node) override;
    void UpdateBaseUnit(int default_unit) override { root_->unit = default_unit; }

protected:
    bool UpdateRule(Node* node, bool value) override; // Cash = 0, Monthly = 1
    bool IsReferenced(int node_id, CString& message) override;
    void ConstructTree() override;
    bool UpdateUnit(Node* node, int value) override;

private:
    bool UpdatePaymentPeriod(Node* node, double value, CString& field = PAYMENT_PERIOD);
    bool UpdateTaxRate(Node* node, double value, CString& field = TAX_RATE);
    bool UpdateDeadline(Node* node, int value, CString& field = DEADLINE);
    bool UpdateEmployee(Node* node, int value, CString& field = EMPLOYEE);
};

#endif // TREEMODELSTAKEHOLDER_H
