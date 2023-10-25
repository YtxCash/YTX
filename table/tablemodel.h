#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include "tableinfo.h"
#include "transaction.h"
#include <QAbstractItemModel>
#include <QSqlDatabase>

class TableModel : public QAbstractItemModel {
    Q_OBJECT

public:
    TableModel(const TableInfo& info, QObject* parent = nullptr);
    ~TableModel();

signals:
    void SendReCalculate(int node_id);

    void SendUpdate(const Transaction& transaction);
    void SendRemove(int node_id, int trans_id);
    void SendCopy(int node_id, const Transaction& transaction);

public slots:
    void ReceiveRule(int node_id, bool rule);

    void ReceiveUpdate(const Transaction& transaction);
    void ReceiveRemove(int node_id, int trans_id);
    void ReceiveCopy(int node_id, const Transaction& transaction);

    void ReceiveReload(int node_id);

    void ReceiveDocument(const QSharedPointer<Transaction>& transaction);

public:
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

public:
    bool insertRow(int row, const QModelIndex& parent = QModelIndex());
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());

public:
    int EmptyTransfer() const;
    int GetRow(int trans_id) const;
    QSharedPointer<Transaction> GetTransaction(const QModelIndex& index) const;

private:
    void CreateTable(int node_id, QList<QSharedPointer<Transaction>>& list);
    bool InsertRecord(int node_id, QSharedPointer<Transaction>& transaction);
    bool RemoveRecord(int trans_id);
    bool UpdateRecord(const QSharedPointer<Transaction>& transaction);

    bool DBTransaction(auto Function);

    bool UpdatePostDate(QSharedPointer<Transaction>& transaction, const QDate& value);
    bool UpdateDescription(QSharedPointer<Transaction>& transaction, const QString& value);
    bool UpdateTransfer(QSharedPointer<Transaction>& transaction, int new_node_id);
    bool UpdateStatus(QSharedPointer<Transaction>& transaction, bool status);
    bool UpdateDebit(QSharedPointer<Transaction>& transaction, double value);
    bool UpdateCredit(QSharedPointer<Transaction>& transaction, double value);

    double CalculateBalance(bool rule, double debit, double credit);
    void AccumulateBalance(QList<QSharedPointer<Transaction>>& list, int row, bool rule);

    void RecycleTransaction(QList<QSharedPointer<Transaction>>& list);
    void RemoveEmptyTransfer(QList<QSharedPointer<Transaction>>& list);

private:
    QList<QSharedPointer<Transaction>> transaction_list_ {};

    QSqlDatabase db_ {};
    TableInfo table_info_;
    QStringList header_ {};
};

#endif // TABLEMODEL_H
