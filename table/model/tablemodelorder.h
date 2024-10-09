#ifndef TABLEMODELORDER_H
#define TABLEMODELORDER_H

#include "tablemodel.h"
#include "tree/model/treemodel.h"

class TableModelOrder final : public TableModel {
    Q_OBJECT

public:
    TableModelOrder(SPSqlite sql, bool rule, int node_id, CInfo& info, TreeModel* product, QObject* parent = nullptr);
    ~TableModelOrder() override = default;

signals:
    void SUpdateFirst(int node_id, double first_diff);

public slots:
    void RUpdateNodeID(int node_id);

public:
    // implemented functions
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

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
            qWarning() << "Failed to update field:" << e.what();
            return false;
        }

        return true;
    }

    bool UpdateInsideProduct(TransShadow* trans_shadow, int value);

    bool UpdateUnitPrice(TransShadow* trans_shadow, double value);
    bool UpdateDiscountPrice(TransShadow* trans_shadow, double value);
    bool UpdateSecond(TransShadow* trans_shadow, double value);

private:
    TreeModel* product_ {};
};

#endif // TABLEMODELORDER_H
