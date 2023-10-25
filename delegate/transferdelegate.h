#ifndef TRANSFERDELEGATE_H
#define TRANSFERDELEGATE_H

#include <QStyledItemDelegate>

class TransferDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    TransferDelegate(const QMultiMap<QString, int>& leaf_map, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

public slots:
    void ReceiveLeaf(const QMultiMap<QString, int>& leaf_map);

private:
    QMultiMap<QString, int> leaf_map_ {};
};

#endif // TRANSFERDELEGATE_H
