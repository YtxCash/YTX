#ifndef ORDERNAMER_H
#define ORDERNAMER_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/abstracttreemodel.h"

class OrderName : public StyledItemDelegate {
public:
    OrderName(const AbstractTreeModel& model, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const AbstractTreeModel& model_;
};

#endif // ORDERNAMER_H
