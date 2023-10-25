#ifndef TREEMODEL_H
#define TREEMODEL_H

#include "node.h"
#include "treeinfo.h"
#include <QAbstractItemModel>
#include <QSqlDatabase>

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    TreeModel(const TreeInfo& tree_info, QObject* parent = nullptr);
    ~TreeModel();

signals:
    void SendRule(int node_id, bool rule);
    void SendLeaf(const QMultiMap<QString, int>& leaf_map);

public slots:
    void ReceiveReCalculate(int node_id);
    void ReceiveUpdate(const Node* node);

    bool ReceiveDelete(int node_id);
    bool ReceiveReplace(int old_node_id, int new_node_id);

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

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

public:
    bool insertRow(int row, const QModelIndex& parent, Node* node);
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());

public:
    Node* GetNode(int node_id) const;
    QMultiMap<QString, int> GetLeafMap() const;
    QString GetLeaf(int node_id) const;
    bool UsageOfNode(int node_id) const;

private:
    bool InsertRecord(int parent_id, Node* node);
    bool RemoveRecord(int id);
    bool DragRecord(int destination_parent_id, int id);
    bool IsDescendant(Node* lhs, Node* rhs) const;

    bool UpdatePlaceholder(Node* node, const bool& value);
    bool UpdateRule(Node* node, const bool& value);
    bool UpdateDescription(Node* node, const QString& value);
    bool UpdateRecord(const Node* node);

    void CreateTree(QHash<int, Node*>& node_hash, QMultiMap<QString, int>& leaf_map);
    void AddIsolatedNode(QSqlQuery& query, QHash<int, Node*>& node_hash);
    void CreateNodeHash(QSqlQuery& query, QHash<int, Node*>& node_hash);
    bool DBTransaction(auto Function);

    void SortIterative(Node* node, const auto& Compare);

    void CreatePathForAll(const QHash<int, Node*>& node_hash, QMultiMap<QString, int>& leaf_map);
    QString CreatePathForLeaf(const Node* node);

    double CalculateLeafTotal(int id, bool rule);
    void CalculatePlaceholderTotal(const Node* node, double value);
    void RecalculateAllTotal(QHash<int, Node*>& node_hash);

    Node* GetNode(const QModelIndex& index) const;

private:
    Node* root_ {};
    QSqlDatabase db_ {};
    TreeInfo tree_info_ {};
    QStringList header_ {};

    QHash<int, Node*> node_hash_ {};
    QMultiMap<QString, int> leaf_map_ {};
};

#endif // TREEMODEL_H
