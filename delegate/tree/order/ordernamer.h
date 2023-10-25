#ifndef ORDERNAMER_H
#define ORDERNAMER_H

#include <QStyledItemDelegate>

#include "tree/model/treemodel.h"

class OrderName : public QStyledItemDelegate {
public:
    OrderName(const TreeModel& model, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const TreeModel& model_;
};

#endif // ORDERNAMER_H
