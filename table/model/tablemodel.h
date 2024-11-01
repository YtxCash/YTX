#ifndef TABLEMODEL_H
#define TABLEMODEL_H

// default implementations are for finance.

#include <QAbstractItemModel>
#include <QMutex>

#include "database/sqlite/sqlite.h"

class TableModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~TableModel();

protected:
    TableModel(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);

signals:
    // send to tree model
    void SUpdateLeafValueTO(int node_id, double diff, CString& node_field);
    void SUpdateLeafValueFPTO(
        int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double settled_diff = 0.0);
    void SSearch();

    // send to signal station
    void SAppendOneTrans(Section section, const TransShadow* trans_shadow);
    void SRemoveOneTrans(Section section, int node_id, int trans_id);
    void SUpdateBalance(Section section, int node_id, int trans_id);

    // send to its table view
    void SResizeColumnToContents(int column);

public slots:
    // receive from sql
    void RRemoveMultiTrans(const QMultiHash<int, int>& node_trans);
    void RMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list);

    // receive from tree model
    void RRule(int node_id, bool rule);

    // receive from signal station
    void RAppendOneTrans(const TransShadow* trans_shadow);
    void RRemoveOneTrans(int node_id, int trans_id);
    void RUpdateBalance(int node_id, int trans_id);

public:
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

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    virtual int GetNodeRow(int node_id) const;

    QModelIndex GetIndex(int trans_id) const;
    QStringList* GetDocumentPointer(const QModelIndex& index) const;

    void UpdateAllState(Check state);

protected:
    // virtual functions
    virtual bool UpdateDebit(TransShadow* trans_shadow, double value);
    virtual bool UpdateCredit(TransShadow* trans_shadow, double value);
    virtual bool UpdateRatio(TransShadow* trans_shadow, double value);

    virtual bool RemoveMultiTrans(const QSet<int>& trans_id_list); // just remove trnas_shadow, keep trans
    virtual bool AppendMultiTrans(int node_id, const QList<int>& trans_id_list);

protected:
    Sqlite* sql_ {};
    bool rule_ {};

    CInfo& info_;
    int node_id_ {};
    QMutex mutex_ {};

    QList<TransShadow*> trans_shadow_list_ {};
};

using PTableModel = QPointer<TableModel>;

#endif // TABLEMODEL_H
