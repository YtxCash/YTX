#ifndef SEARCHTRANSMODEL_H
#define SEARCHTRANSMODEL_H

#include <QAbstractItemModel>

#include "component/info.h"
#include "database/sqlite/sqlite.h"

class SearchTransModel final : public QAbstractItemModel {
    Q_OBJECT
public:
    SearchTransModel(CInfo* info, SPSqlite sql, QObject* parent = nullptr);
    ~SearchTransModel();

public:
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

public:
    void Query(const QString& text);

private:
    SPSqlite sql_ {};

    TransList trans_list_ {};
    CInfo* info_ {};
};

#endif // SEARCHTRANSMODEL_H
