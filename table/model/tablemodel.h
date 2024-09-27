#ifndef TABLEMODEL_H
#define TABLEMODEL_H

// default implementations are for finance.

#include <QAbstractItemModel>

#include "component/enumclass.h"
#include "component/info.h"
#include "component/settings.h"
#include "component/using.h"
#include "database/sqlite/sqlite.h"
#include "table/trans.h"

class TableModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~TableModel();

protected:
    TableModel(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);

signals:
    // send to tree model
    void SUpdateOneTotal(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff);
    void SSearch();

    // send to signal station
    void SAppendOne(Section section, const TransShadow* trans_shadow);
    void SRemoveOne(Section section, int node_id, int trans_id);
    void SUpdateBalance(Section section, int node_id, int trans_id);
    void SMoveMulti(Section section, int old_node_id, int new_node_id, const QList<int>& trans_id_list);

    // send to its table view
    void SResizeColumnToContents(int column);

    // send to edit/insert order dialog
    void SUpdateOrderDescription(const QString& value);

public slots:
    // receive from sql
    virtual void RRemoveMulti(const QMultiHash<int, int>& node_trans);

    // receive from tree model
    void RNodeRule(int node_id, bool node_rule);

    // receive from signal station
    void RAppendOne(const TransShadow* trans_shadow);
    void RRetrieveOne(TransShadow* trans_shadow);
    void RRemoveOne(int node_id, int trans_id);
    void RUpdateBalance(int node_id, int trans_id);
    void RMoveMulti(int old_node_id, int new_node_id, const QList<int>& trans_id_list);

public:
    // virtual functions
    virtual bool RemoveTrans(int row, const QModelIndex& parent = QModelIndex());

    // implemented functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    void sort(int column, Qt::SortOrder order) override;

    // member functions
    bool InsertTrans(const QModelIndex& parent = QModelIndex());

    int GetRow(int node_id) const;
    QModelIndex GetIndex(int trans_id) const;
    QStringList* GetDocumentPointer(const QModelIndex& index) const;

    void UpdateAllState(Check state);

protected:
    // virtual functions
    virtual bool UpdateDebit(TransShadow* trans_shadow, double value);
    virtual bool UpdateCredit(TransShadow* trans_shadow, double value);
    virtual bool UpdateRatio(TransShadow* trans_shadow, double value);

    virtual bool RemoveMulti(const QList<int>& trans_id_list); // just remove trnas_shadow, keep related trans
    virtual bool InsertMulti(int node_id, const QList<int>& trans_id_list);

    // member functions
    double Balance(bool node_rule, double debit, double credit) { return (node_rule ? 1 : -1) * (credit - debit); }
    void AccumulateSubtotal(int start, bool node_rule);

    bool UpdateRelatedNode(TransShadow* trans_shadow, int value);

protected:
    template <typename T>
    bool UpdateField(TransShadow* trans_shadow, const T& value, CString& field, T* TransShadow::* member, const std::function<void()>& action = {})
    {
        if (trans_shadow == nullptr || trans_shadow->*member == nullptr)
            return false;

        if (*(trans_shadow->*member) == value)
            return false;

        *(trans_shadow->*member) = value;

        if (*trans_shadow->related_node != 0) {
            try {
                sql_->UpdateField(info_.transaction, value, field, *trans_shadow->id);

                if (action)
                    action();
            } catch (const std::exception& e) {
                qWarning() << "Failed to update field:" << e.what();
                return false;
            }
        }

        return true;
    }

protected:
    SPSqlite sql_ {};
    bool node_rule_ {};

    CInfo& info_;
    CSectionRule& section_rule_;
    const int node_id_ {};

    QList<TransShadow*> trans_shadow_list_ {};
};

#endif // TABLEMODEL_H
