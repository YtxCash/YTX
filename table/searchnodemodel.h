#ifndef SEARCHNODEMODEL_H
#define SEARCHNODEMODEL_H

#include <QAbstractItemModel>

#include "component/info.h"
#include "component/using.h"
#include "tree/model/treemodel.h"

class SearchNodeModel final : public QAbstractItemModel {
    Q_OBJECT
public:
    SearchNodeModel(CInfo& info, const TreeModel* tree_model, Sqlite* sql, QObject* parent = nullptr);
    ~SearchNodeModel() = default;

public:
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

public:
    void Query(CString& text);

private:
    Sqlite* sql_ {};

    CInfo& info_;
    const TreeModel* tree_model_;

    QList<const Node*> node_list_ {};
};

#endif // SEARCHNODEMODEL_H
