#ifndef TABLEMODELORDER_H
#define TABLEMODELORDER_H

#include "tablemodel.h"
#include "tree/model/treemodel.h"

class TableModelOrder final : public TableModel {
    Q_OBJECT

public:
    TableModelOrder(
        SPSqlite sql, bool rule, int node_id, int party_id, CInfo& info, const TreeModel* product_tree, SPSqlite sqlite_stakeholder, QObject* parent = nullptr);
    ~TableModelOrder() override;

public slots:
    void RUpdateNodeID(int node_id);
    void RUpdatePartyID(int party_id);
    void RUpdateLocked(int node_id, bool checked);

public:
    // implemented functions
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

    int GetNodeRow(int node_id) const override;

private:
    template <typename T>
    bool UpdateField(TransShadow* trans_shadow, const T& value, CString& field, T* TransShadow::* member, const std::function<void()>& action = {})
    {
        if (trans_shadow == nullptr || trans_shadow->*member == nullptr || *(trans_shadow->*member) == value)
            return false;

        *(trans_shadow->*member) = value;

        if (*trans_shadow->node_id == 0 || *trans_shadow->lhs_node == 0)
            return false;

        try {
            sql_->UpdateField(info_.transaction, value, field, *trans_shadow->id);
            if (action)
                action();
        } catch (const std::exception& e) {
            qWarning() << "Failed in UpdateField" << e.what();
            return false;
        }

        return true;
    }

    bool UpdateInsideProduct(TransShadow* trans_shadow, int value);
    bool UpdateOutsideProduct(TransShadow* trans_shadow, int value);

    bool UpdateUnitPrice(TransShadow* trans_shadow, double value);
    bool UpdateDiscountPrice(TransShadow* trans_shadow, double value);
    bool UpdateSecond(TransShadow* trans_shadow, double value);

    void SearchExclusivePrice(TransShadow* trans_shadow, int inside_product_id, int outside_product_id);

private:
    const TreeModel* product_tree_ {};
    SPSqlite sqlite_stakeholder_ {};
    QList<TransShadow*> stakeholder_trans_shadow_list_ {};
    QHash<int, double> update_exclusive_price_ {}; // inside_product_id, exclusive_price
    int party_id_ {};
};

#endif // TABLEMODELORDER_H
